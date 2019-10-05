/*
 * Copyright (C) 2019 Medusalix
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "WirelessOneMT76.h"
#include "WirelessOneDongle.h"

#include <IOKit/usb/IOUSBHostInterface.h>
#include <libkern/OSKextLib.h>

#define LOG(format, ...) IOLog("WirelessOne (MT76): %s - " format "\n", __func__, ##__VA_ARGS__)
#define LOG_MAC(format, m) LOG(format "%02X:%02X:%02X:%02X:%02X:%02X", m[0], m[1], m[2], m[3], m[4], m[5])

OSDefineMetaClassAndStructors(WirelessOneMT76, IOService)

bool WirelessOneMT76::start(IOService *provider)
{
    if (!IOService::start(provider))
    {
        LOG("super failed");
        
        return false;
    }
    
    device = OSDynamicCast(IOUSBHostDevice, provider);
    
    if (!device)
    {
        LOG("invalid provider");
        stop(provider);
        
        return false;
    }
    
    if (!device->open(this))
    {
        LOG("failed to open device");
        stop(provider);
        
        return false;
    }
    
    IOReturn error = device->setConfiguration(1);
    
    if (error != kIOReturnSuccess)
    {
        LOG("error setting configuration: 0x%.8x", error);
        stop(provider);
        
        return false;
    }
    
    OSIterator *iterator = device->getChildIterator(gIOServicePlane);
    
    if (!iterator)
    {
        LOG("missing child iterator");
        stop(provider);
        
        return false;
    }
    
    OSObject *object = nullptr;
    
    while ((object = iterator->getNextObject()))
    {
        interface = OSDynamicCast(IOUSBHostInterface, object);
        
        if (interface)
        {
            break;
        }
    }
    
    iterator->release();
    
    if (!interface)
    {
        LOG("missing host interface");
        stop(provider);
        
        return false;
    }
    
    if (!interface->open(this))
    {
        LOG("failed to open interface");
        stop(provider);
        
        return false;
    }
    
    const ConfigurationDescriptor *configInfo = interface->getConfigurationDescriptor();
    const InterfaceDescriptor *interfaceInfo = interface->getInterfaceDescriptor();
    
    if (!configInfo || !interfaceInfo)
    {
        LOG("missing descriptors");
        stop(provider);
        
        return false;
    }
    
    const EndpointDescriptor *endpointInfo = nullptr;
    
    while ((endpointInfo = getNextEndpointDescriptor(configInfo, interfaceInfo, endpointInfo)))
    {
        uint8_t type = getEndpointType(endpointInfo);
        
        if (type != kEndpointTypeBulk)
        {
            continue;
        }
        
        uint8_t number = getEndpointNumber(endpointInfo);
        uint8_t direction = getEndpointDirection(endpointInfo);
        
        if (direction == kEndpointDirectionIn)
        {
            if (number == MT_EP_READ)
            {
                readPipe = interface->copyPipe(endpointInfo->bEndpointAddress);
            }
            
            else if (number == MT_EP_READ_PACKET)
            {
                packetPipe = interface->copyPipe(endpointInfo->bEndpointAddress);
            }
        }
        
        else if (direction == kEndpointDirectionOut && number == MT_EP_WRITE)
        {
            writePipe = interface->copyPipe(endpointInfo->bEndpointAddress);
        }
    }
    
    if (!readPipe || !writePipe || !packetPipe)
    {
        LOG("missing input/output pipes");
        stop(provider);
        
        return false;
    }
    
    resourceLock = IOLockAlloc();
    
    if (!resourceLock)
    {
        LOG("failed to acquire lock");
        stop(provider);
        
        return false;
    }
    
    workLoop = getWorkLoop();
    
    if (!workLoop)
    {
        LOG("missing work loop");
        stop(provider);
        
        return false;
    }
    
    timer = IOTimerEventSource::timerEventSource(
        this,
        OSMemberFunctionCast(IOTimerEventSource::Action, this, &WirelessOneMT76::timerAction)
    );
    
    if (!timer)
    {
        LOG("failed to create timer");
        stop(provider);
        
        return false;
    }
    
    error = workLoop->addEventSource(timer);
    
    if (error != kIOReturnSuccess)
    {
        LOG("error adding timer: 0x%.8x", error);
        stop(provider);
        
        return false;
    }
    
    if (!requestFirmware())
    {
        LOG("failed to request firmware");
        stop(provider);
        
        return false;
    }
    
    if (!loadFirmware())
    {
        LOG("failed to load firmware");
        stop(provider);
        
        return false;
    }
    
    if (!initChip())
    {
        LOG("failed to init chip");
        stop(provider);
        
        return false;
    }
    
    if (!initDongle())
    {
        LOG("failed to init dongle");
        stop(provider);
        
        return false;
    }
    
    return true;
}

void WirelessOneMT76::stop(IOService *provider)
{
    dispose();
    
    IOService::stop(provider);
}

bool WirelessOneMT76::willTerminate(IOService *provider, IOOptionBits options)
{
    dispose();
    
    return IOService::willTerminate(provider, options);
}

bool WirelessOneMT76::initChip()
{
    // Select RX ring buffer 1
    // Turn radio on
    // Load BBP command register
    if (!selectFunction(Q_SELECT, 1) ||
        !powerMode(RADIO_ON) ||
        !loadCr(MT_RF_BBP_CR)
    ) {
        LOG("failed to init radio");
        
        return false;
    }
    
    // Write initial register values
    if (!initRegisters())
    {
        LOG("failed to init registers");
        
        return false;
    }
    
    controlWrite(MT_MAC_SYS_CTRL, 0);
    
    // Calibrate chip
    if (!calibrate(MCU_CAL_TEMP_SENSOR, 0) ||
        !calibrate(MCU_CAL_RXDCOC, 1) ||
        !calibrate(MCU_CAL_RC, 0)
    ) {
        LOG("failed to calibrate chip");
        
        return false;
    }
        
    controlWrite(MT_MAC_SYS_CTRL, MT_MAC_SYS_CTRL_ENABLE_TX | MT_MAC_SYS_CTRL_ENABLE_RX);
    
    // Set default channel
    if (!switchChannel(MT_CHANNEL))
    {
        LOG("failed to set channel");
        
        return false;
    }
    
    // Start beacon transmission
    if (!enableBeacon())
    {
        LOG("failed to enable beacon");
        
        return false;
    }
    
    return true;
}

bool WirelessOneMT76::initDongle()
{
    dongle = new WirelessOneDongle;
    
    if (!dongle->init())
    {
        LOG("failed to init dongle");
        
        return false;
    }
    
    if (!dongle->attach(this))
    {
        LOG("failed to attach dongle");
        
        return false;
    }
    
    dongle->registerService();
    
    if (!dongle->start(this))
    {
        LOG("failed to start dongle");
        
        return false;
    }
    
    if (!startBulkRead(readPipe))
    {
        LOG("failed to start bulk read");
        
        return false;
    }
    
    if (!startBulkRead(packetPipe))
    {
        LOG("failed to start bulk packet read");
        
        return false;
    }
    
    return true;
}

bool WirelessOneMT76::requestFirmware()
{
    IOLockLock(resourceLock);
    
    OSKextRequestResource(
        OSKextGetCurrentIdentifier(),
        MT_FW_RESOURCE,
        requestResourceCallback,
        this,
        nullptr
    );
    
    IOLockSleep(resourceLock, this, 0);
    IOLockUnlock(resourceLock);
    
    return firmware;
}

bool WirelessOneMT76::loadFirmware()
{
    DmaConfig config = {};
    
    config.rxBulkEnabled = 1;
    config.txBulkEnabled = 1;
    
    // Configure direct memory access (DMA)
    // Enable FCE and packet DMA
    controlWrite(MT_USB_U3DMA_CFG, config.value, MT_VEND_WRITE_CFG);
    controlWrite(MT_FCE_PSE_CTRL, 0x01);
    controlWrite(MT_TX_CPU_FROM_FCE_BASE_PTR, 0x400230);
    controlWrite(MT_TX_CPU_FROM_FCE_MAX_COUNT, 0x01);
    controlWrite(MT_TX_CPU_FROM_FCE_CPU_DESC_IDX, 0x01);
    controlWrite(MT_FCE_PDMA_GLOBAL_CONF, 0x44);
    controlWrite(MT_FCE_SKIP_FS, 0x03);
    
    FwHeader *header = (FwHeader*)firmware->getBytesNoCopy();
    uint8_t *ilmStart = (uint8_t*)header + sizeof(FwHeader);
    uint8_t *dlmStart = ilmStart + header->ilmLength;
    uint8_t *dlmEnd = dlmStart + header->dlmLength;
    
    // Upload firmware parts (ILM and DLM)
    if (!loadFirmwarePart(MT_MCU_ILM_OFFSET, ilmStart, dlmStart))
    {
        LOG("failed to write ILM");
        
        return false;
    }
    
    if (!loadFirmwarePart(MT_MCU_DLM_OFFSET, dlmStart, dlmEnd))
    {
        LOG("failed to write DLM");
        
        return false;
    }
    
    // Load initial vector block (IVB)
    controlWrite(MT_FCE_DMA_ADDR, 0, MT_VEND_WRITE_CFG);
    controlWrite(MT_FW_LOAD_IVB, 0, MT_VEND_DEV_MODE);
    
    // Start timer
    timeout = false;
    timer->setTimeoutMS(MT_TIMEOUT);
    
    // Wait for firmware to start
    while (controlRead(MT_FCE_DMA_ADDR, MT_VEND_READ_CFG) != 0x01 && !timeout);
    
    if (timeout)
    {
        LOG("timeout");
        
        return false;
    }
    
    LOG("firmware loaded");
    timer->cancelTimeout();
    
    return true;
}

bool WirelessOneMT76::loadFirmwarePart(uint32_t offset, uint8_t *start, uint8_t *end)
{
    // Send firmware in chunks
    for (uint8_t *chunk = start; chunk < end; chunk += MT_FW_CHUNK_SIZE)
    {
        uint32_t address = (uint32_t)(offset + chunk - start);
        uint32_t remaining = (uint32_t)(end - chunk);
        uint16_t length = remaining > MT_FW_CHUNK_SIZE ? MT_FW_CHUNK_SIZE : remaining;
        
        TxInfoCommand info = {};
        
        info.port = CPU_TX_PORT;
        info.infoType = NORMAL_PACKET;
        info.length = length;
        
        uint32_t footer = 0;
        OSData *data = OSData::withCapacity(sizeof(info) + length + sizeof(footer));
        
        data->appendBytes((uint8_t*)&info, sizeof(info));
        data->appendBytes(chunk, length);
        data->appendBytes((uint8_t*)&footer, sizeof(footer));
        
        controlWrite(MT_FCE_DMA_ADDR, address, MT_VEND_WRITE_CFG);
        controlWrite(MT_FCE_DMA_LEN, length << 16, MT_VEND_WRITE_CFG);
        
        if (!bulkWrite(data))
        {
            LOG("failed to write firmware part");
            data->release();
            
            return false;
        }
        
        data->release();
        
        uint32_t complete = (length << 16) | MT_DMA_COMPLETE;
        
        // Start timer
        timeout = false;
        timer->setTimeoutMS(MT_TIMEOUT);
        
        while (controlRead(MT_FCE_DMA_LEN, MT_VEND_READ_CFG) != complete && !timeout);
        
        if (timeout)
        {
            LOG("timeout");
            
            return false;
        }
        
        timer->cancelTimeout();
    }
    
    return true;
}

bool WirelessOneMT76::initRegisters()
{
    controlWrite(MT_MAC_SYS_CTRL, MT_MAC_SYS_CTRL_RESET_CSR | MT_MAC_SYS_CTRL_RESET_BBP);
    controlWrite(MT_USB_DMA_CFG, 0);
    controlWrite(MT_MAC_SYS_CTRL, 0);
    controlWrite(MT_PWR_PIN_CFG, 0);
    controlWrite(MT_LDO_CTRL_1, 0x6b006464);
    controlWrite(MT_WPDMA_GLO_CFG, 0x70);
    controlWrite(MT_WMM_AIFSN, 0x2273);
    controlWrite(MT_WMM_CWMIN, 0x2344);
    controlWrite(MT_WMM_CWMAX, 0x34aa);
    controlWrite(MT_FCE_DMA_ADDR, 0x041200);
    controlWrite(MT_TSO_CTRL, 0);
    controlWrite(MT_PBF_SYS_CTRL, 0x080c00);
    controlWrite(MT_PBF_TX_MAX_PCNT, 0x1fbf1f1f);
    controlWrite(MT_FCE_PSE_CTRL, 0x01);
    controlWrite(MT_MAC_SYS_CTRL, MT_MAC_SYS_CTRL_ENABLE_TX | MT_MAC_SYS_CTRL_ENABLE_RX);
    controlWrite(MT_AUTO_RSP_CFG, 0x13);
    controlWrite(MT_MAX_LEN_CFG, 0x3e3fff);
    controlWrite(MT_AMPDU_MAX_LEN_20M1S, 0xfffc9855);
    controlWrite(MT_AMPDU_MAX_LEN_20M2S, 0xff);
    controlWrite(MT_BKOFF_SLOT_CFG, 0x0109);
    controlWrite(MT_EDCA_CFG_AC(0), 0x064320);
    controlWrite(MT_EDCA_CFG_AC(1), 0x0a4700);
    controlWrite(MT_EDCA_CFG_AC(2), 0x043238);
    controlWrite(MT_EDCA_CFG_AC(3), 0x03212f);
    controlWrite(MT_TX_PIN_CFG, 0x150f0f);
    controlWrite(MT_TX_SW_CFG0, 0x101001);
    controlWrite(MT_TX_SW_CFG1, 0x010000);
    controlWrite(MT_TXOP_CTRL_CFG, 0x583f);
    controlWrite(MT_TX_RTS_CFG, 0x092b20);
    controlWrite(MT_TX_TIMEOUT_CFG, 0x0a0f90);
    controlWrite(MT_TX_RETRY_CFG, 0x47d01f0f);
    controlWrite(MT_CCK_PROT_CFG, 0x03f40003);
    controlWrite(MT_OFDM_PROT_CFG, 0x03f40003);
    controlWrite(MT_MM20_PROT_CFG, 0x01742004);
    controlWrite(MT_GF20_PROT_CFG, 0x01742004);
    controlWrite(MT_GF40_PROT_CFG, 0x03f42084);
    controlWrite(MT_EXP_ACK_TIME, 0x2c00dc);
    controlWrite(MT_TX0_RF_GAIN_ATTEN, 0x22160a00);
    controlWrite(MT_TX_ALC_CFG_3, 0x22160a76);
    controlWrite(MT_TX_ALC_CFG_0, 0x3f3f1818);
    controlWrite(MT_TX_ALC_CFG_4, 0x80000606);
    controlWrite(MT_TX_PROT_CFG6, 0xe3f52004);
    controlWrite(MT_TX_PROT_CFG7, 0xe3f52084);
    controlWrite(MT_TX_PROT_CFG8, 0xe3f52104);
    controlWrite(MT_PIFS_TX_CFG, 0x060fff);
    controlWrite(MT_RX_FILTR_CFG, 0x015f9f);
    controlWrite(MT_LEGACY_BASIC_RATE, 0x017f);
    controlWrite(MT_HT_BASIC_RATE, 0x8003);
    controlWrite(MT_PN_PAD_MODE, 0x02);
    controlWrite(MT_TXOP_HLDR_ET, 0x02);
    controlWrite(MT_TX_PROT_CFG6, 0xe3f42004);
    controlWrite(MT_TX_PROT_CFG7, 0xe3f42084);
    controlWrite(MT_TX_PROT_CFG8, 0xe3f42104);
    controlWrite(MT_DACCLK_EN_DLY_CFG, 0);
    controlWrite(MT_RF_PA_MODE_ADJ0, 0xee000000);
    controlWrite(MT_RF_PA_MODE_ADJ1, 0xee000000);
    controlWrite(MT_TX0_RF_GAIN_CORR, 0x0f3c3c3c);
    controlWrite(MT_TX1_RF_GAIN_CORR, 0x0f3c3c3c);
    controlWrite(MT_PBF_CFG, 0x1efebcf5);
    controlWrite(MT_PAUSE_ENABLE_CONTROL1, 0x0a);
    controlWrite(MT_XIFS_TIME_CFG, 0x33a40e0a);
    controlWrite(MT_FCE_L2_STUFF, 0x03ff0223);
    controlWrite(MT_TX_RTS_CFG, 0);
    controlWrite(MT_BEACON_TIME_CFG, 0x0640);
    controlWrite(MT_TXOP_CTRL_CFG, 0x583f);
    controlWrite(MT_CMB_CTRL, 0x0091a7ff);
    
    // Read crystal calibration from EFUSE
    uint32_t calibration = efuseRead(MT_EF_XTAL_CALIB, 3) >> 16;
    
    controlWrite(MT_XO_CTRL5, calibration, MT_VEND_WRITE_CFG);
    controlWrite(MT_XO_CTRL6, MT_XO_CTRL6_C2_CTRL, MT_VEND_WRITE_CFG);
    
    // Read MAC address from EFUSE
    uint32_t macAddress1 = efuseRead(MT_EF_MAC_ADDR, 1);
    uint32_t macAddress2 = efuseRead(MT_EF_MAC_ADDR, 2);
    
    memcpy(macAddress, &macAddress1, 4);
    memcpy(macAddress + 4, &macAddress2, 2);
    
    if (!burstWrite(MT_MAC_ADDR_DW0, macAddress, sizeof(macAddress)))
    {
        LOG("failed to write MAC address");
        
        return false;
    }
    
    if (!burstWrite(MT_MAC_BSSID_DW0, macAddress, sizeof(macAddress)))
    {
        LOG("failed to write BSSID");
        
        return false;
    }
    
    LOG_MAC("MAC address: ", macAddress);
    
    return true;
}

bool WirelessOneMT76::enableBeacon()
{
    // Required for controllers to connect reliably
    // Probably contains the selected channel pair
    // 00 -> a5 and 30 -> 99
    uint8_t beaconData[] = {
        0xdd, 0x10, 0x00, 0x50,
        0xf2, 0x11, 0x01, 0x10,
        0x00, 0xa5, 0x30, 0x99,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00
    };
    BeaconFrame beacon = {};
    
    // Default beacon interval
    // Original capability info
    // Wildcard SSID
    beacon.interval = MT_BCN_DEF_INTVAL;
    beacon.capabilityInfo = 0xc631;
    
    OSData *out = OSData::withCapacity(sizeof(beacon) + sizeof(beaconData));
    
    out->appendBytes(&beacon, sizeof(beacon));
    out->appendBytes(beaconData, sizeof(beaconData));
    
    if (!writeBeacon(MT_BCN_NORMAL_OFFSET, MT_WLAN_BEACON, out))
    {
        LOG("failed to write beacon");
        out->release();
        
        return false;
    }
    
    out->release();
    
    // This data is VERY IMPORTANT
    // The first value always has to be 0x01
    // I suspect it's a list of channel pairs the AP switches through
    // Each pair contains a TX and a RX channel
    // I wonder why some channels are listed twice though...
    const uint32_t gain1[] = {
        0x01, 0xa5, 0x0b, 0x01,
        0x06, 0x0b, 0x24, 0x28,
        0x2c, 0x30, 0x95, 0x99,
        0x9d, 0xa1
    };
    const uint32_t gain2[] = { 0x08 };
    
    if (!initGain(0, macAddress, sizeof(macAddress)) ||
        !initGain(7, (uint8_t*)gain1, sizeof(gain1)) ||
        !initGain(14, (uint8_t*)gain2, sizeof(gain2))
    ) {
        LOG("failed to init beacon gain");
        
        return false;
    }
    
    if (!calibrate(MCU_CAL_RXDCOC, 0))
    {
        LOG("failed to calibrate beacon");
        
        return false;
    }
    
    LOG("beacon started");
    
    return true;
}

void WirelessOneMT76::handleWlanPacket(uint8_t data[])
{
    WlanFrame *wlanFrame = (WlanFrame*)data;
    
    // Packet has wrong destination address
    if (memcmp(wlanFrame->destination, macAddress, sizeof(macAddress)))
    {
        return;
    }
    
    uint8_t type = wlanFrame->frameControl.type;
    uint8_t subtype = wlanFrame->frameControl.subtype;
    
    if (type != MT_WLAN_MGMT)
    {
        return;
    }
    
    if (subtype == MT_WLAN_ASSOC_REQ)
    {
        LOG_MAC("controller associating: ", wlanFrame->source);
        
        if (!associateController(wlanFrame->source))
        {
            LOG("failed to associate controller");
            
            return;
        }
        
        dongle->handleConnect(wlanFrame->source);
    }
    
    else if (subtype == MT_WLAN_DISASSOC)
    {
        LOG_MAC("controller disassociating: ", wlanFrame->source);
        
        dongle->handleDisconnect(wlanFrame->source);
    }
    
    else if (subtype == MT_WLAN_PAIR)
    {
        PairingFrame *pairingFrame = (PairingFrame*)(data + sizeof(WlanFrame));
        
        // Pairing frame 'unknown' is always 0x70, 'type' is 0x01 for pairing
        if (pairingFrame->type != 0x01)
        {
            return;
        }
        
        LOG_MAC("controller pairing: ", wlanFrame->source);
        
        if (!pairController(wlanFrame->source))
        {
            LOG("failed to pair controller");
        }
    }
}

void WirelessOneMT76::handleControllerPacket(uint8_t data[])
{
    WlanFrame *wlanFrame = (WlanFrame*)data;
    
    // Packet has wrong destination address
    if (memcmp(wlanFrame->destination, macAddress, sizeof(macAddress)))
    {
        return;
    }
    
    uint8_t type = wlanFrame->frameControl.type;
    uint8_t subtype = wlanFrame->frameControl.subtype;
    
    if (type != MT_WLAN_DATA || subtype != MT_WLAN_QOS_DATA)
    {
        return;
    }
    
    // Skip 2 byte padding
    dongle->handleData(
        wlanFrame->source,
        data + sizeof(WlanFrame) + sizeof(QosFrame) + sizeof(uint16_t)
    );
}

bool WirelessOneMT76::associateController(uint8_t controllerAddress[])
{
    const uint32_t gain[] = { 0x0000, 0x1f40 };
    
    TxWi txWi = {};
    
    // OFDM transmission method
    // Wait for acknowledgement
    // Ignore wireless client index (WCID)
    txWi.phyMode = MT_PHY_TYPE_OFDM;
    txWi.ack = 1;
    txWi.wcid = 0xff;
    txWi.mpduByteCount = sizeof(WlanFrame) + sizeof(AssociationResponseFrame);
    
    WlanFrame wlanFrame = {};
    
    wlanFrame.frameControl.type = MT_WLAN_MGMT;
    wlanFrame.frameControl.subtype = MT_WLAN_ASSOC_RESP;
    
    memcpy(wlanFrame.destination, controllerAddress, sizeof(macAddress));
    memcpy(wlanFrame.source, macAddress, sizeof(macAddress));
    memcpy(wlanFrame.bssId, macAddress, sizeof(macAddress));
    
    // Status code zero (success)
    // Association ID can remain zero
    // Wildcard SSID
    AssociationResponseFrame associationFrame = {};
    
    OSData *out = OSData::withCapacity(sizeof(txWi) + sizeof(wlanFrame) + sizeof(associationFrame));
    
    out->appendBytes(&txWi, sizeof(txWi));
    out->appendBytes(&wlanFrame, sizeof(wlanFrame));
    out->appendBytes(&associationFrame, sizeof(associationFrame));
    
    if (!burstWrite(MT_WCID_ADDR(1), controllerAddress, sizeof(macAddress)))
    {
        LOG("failed to write WCID");
        out->release();
        
        return false;
    }
    
    if (!initGain(1, (uint8_t*)gain, sizeof(gain)))
    {
        LOG("failed to init association gain");
        out->release();
        
        return false;
    }
    
    if (!sendWlanPacket(out))
    {
        LOG("failed to send association packet");
        out->release();
        
        return false;
    }
    
    out->release();
    
    return true;
}

bool WirelessOneMT76::pairController(uint8_t controllerAddress[])
{
    uint8_t data[] = { 0x70, 0x0f, 0x00, 0x45 };
    
    TxWi txWi = {};
    
    // OFDM transmission method
    // Wait for acknowledgement
    // Ignore wireless client index (WCID)
    txWi.phyMode = MT_PHY_TYPE_OFDM;
    txWi.ack = 1;
    txWi.wcid = 0xff;
    txWi.mpduByteCount = sizeof(WlanFrame) + sizeof(data);
    
    WlanFrame wlanFrame = {};
    
    wlanFrame.frameControl.type = MT_WLAN_MGMT;
    wlanFrame.frameControl.subtype = MT_WLAN_PAIR;
    
    memcpy(wlanFrame.destination, controllerAddress, sizeof(macAddress));
    memcpy(wlanFrame.source, macAddress, sizeof(macAddress));
    memcpy(wlanFrame.bssId, macAddress, sizeof(macAddress));
    
    OSData *out = OSData::withCapacity(sizeof(txWi) + sizeof(wlanFrame) + sizeof(data));
    
    out->appendBytes(&txWi, sizeof(txWi));
    out->appendBytes(&wlanFrame, sizeof(wlanFrame));
    out->appendBytes(&data, sizeof(data));
    
    if (!sendWlanPacket(out))
    {
        LOG("failed to send pairing packet");
        out->release();
        
        return false;
    }
    
    if (!setLedMode(0))
    {
        LOG("failed to set pairing LED");
        out->release();
        
        return false;
    }
    
    out->release();
    
    return true;
}

bool WirelessOneMT76::sendWlanPacket(OSData *data)
{
    // Values must be 32-bit aligned
    // 32 zero-bits mark the end
    uint32_t length = data->getLength();
    uint8_t padding = length % sizeof(uint32_t);
    uint32_t footer = 0;
    
    TxInfoPacket info = {};
    
    info.port = WLAN_PORT;
    info.infoType = NORMAL_PACKET;
    info.is80211 = 1;
    info.wiv = 1;
    info.qsel = MT_QSEL_EDCA;
    info.length = length + padding;
    
    OSData *out = OSData::withCapacity(sizeof(info) + info.length + sizeof(footer));
    
    out->appendBytes(&info, sizeof(info));
    out->appendBytes(data);
    out->appendBytes(&footer, padding);
    out->appendBytes(&footer, sizeof(footer));
    
    if (!bulkWrite(out))
    {
        LOG("failed to write WLAN packet");
        out->release();
        
        return false;
    }
    
    out->release();
    
    return true;
}

bool WirelessOneMT76::sendControllerPacket(uint8_t controllerAddress[], OSData *data)
{
    TxWi txWi = {};
    
    // OFDM transmission method
    // Wait for acknowledgement
    txWi.phyMode = MT_PHY_TYPE_OFDM;
    txWi.ack = 1;
    txWi.mpduByteCount = sizeof(WlanFrame) + sizeof(QosFrame) + data->getLength();
    
    WlanFrame wlanFrame = {};
    
    // Frame is sent from AP (DS)
    // Duration is the time required to transmit (μs)
    wlanFrame.frameControl.type = MT_WLAN_DATA;
    wlanFrame.frameControl.subtype = MT_WLAN_QOS_DATA;
    wlanFrame.frameControl.fromDs = 1;
    wlanFrame.duration = 144;
    
    memcpy(wlanFrame.destination, controllerAddress, sizeof(macAddress));
    memcpy(wlanFrame.source, macAddress, sizeof(macAddress));
    memcpy(wlanFrame.bssId, macAddress, sizeof(macAddress));
    
    QosFrame qosFrame = {};
    
    // Frames and data must be 32-bit aligned
    uint32_t length = sizeof(txWi) + sizeof(wlanFrame) + sizeof(qosFrame);
    uint8_t framePadding = length % sizeof(uint32_t);
    uint8_t dataPadding = data->getLength() % sizeof(uint32_t);
    uint64_t header = 0;
    
    OSData *out = OSData::withCapacity(sizeof(header) + length + data->getLength());
    
    out->appendBytes(&header, sizeof(header));
    out->appendBytes(&txWi, sizeof(txWi));
    out->appendBytes(&wlanFrame, sizeof(wlanFrame));
    out->appendBytes(&qosFrame, sizeof(qosFrame));
    out->appendBytes(&header, framePadding);
    out->appendBytes(data);
    out->appendBytes(&header, dataPadding);
    
    if (!sendCommand(CMD_PACKET_TX, out))
    {
        LOG("failed to send controller packet");
        out->release();
        
        return false;
    }
    
    out->release();
    
    return true;
}

bool WirelessOneMT76::selectFunction(McuFunction function, uint32_t value)
{
    OSData *data = OSData::withCapacity(sizeof(function) + sizeof(value));
    
    data->appendBytes(&function, sizeof(function));
    data->appendBytes(&value, sizeof(value));
    
    if (!sendCommand(CMD_FUN_SET_OP, data))
    {
        LOG("failed to select function");
        data->release();
        
        return false;
    }
    
    data->release();
    
    return true;
}

bool WirelessOneMT76::powerMode(McuPowerMode mode)
{
    OSData *data = OSData::withBytes(&mode, sizeof(mode));
    
    if (!sendCommand(CMD_POWER_SAVING_OP, data))
    {
        LOG("failed to set power mode");
        data->release();
        
        return false;
    }
    
    data->release();
    
    return true;
}

bool WirelessOneMT76::loadCr(McuCrMode mode)
{
    OSData *data = OSData::withBytes(&mode, sizeof(mode));
    
    if (!sendCommand(CMD_LOAD_CR, data))
    {
        LOG("failed to load CR");
        data->release();
        
        return false;
    }
    
    data->release();
    
    return true;
}

bool WirelessOneMT76::burstWrite(uint32_t index, uint8_t values[], uint32_t length)
{
    index += MT_BURST_WRITE;
    
    OSData *data = OSData::withCapacity(sizeof(index) + length);
    
    data->appendBytes(&index, sizeof(index));
    data->appendBytes(values, length);
    
    if (!sendCommand(CMD_BURST_WRITE, data))
    {
        LOG("failed to burst write register");
        data->release();
        
        return false;
    }
    
    data->release();
    
    return true;
}

bool WirelessOneMT76::calibrate(McuCalibration calibration, uint32_t value)
{
    OSData *data = OSData::withCapacity(sizeof(calibration) + sizeof(value));
    
    data->appendBytes(&calibration, sizeof(calibration));
    data->appendBytes(&value, sizeof(value));
    
    if (!sendCommand(CMD_CALIBRATION_OP, data))
    {
        LOG("failed to calibrate");
        data->release();
        
        return false;
    }
    
    data->release();
    
    return true;
}

bool WirelessOneMT76::switchChannel(uint8_t channel)
{
    SwitchChannelMessage message = {};
    
    message.channel = channel;
    message.chainmask = 0x0101;
    
    OSData *data = OSData::withCapacity(sizeof(message));
    
    data->appendBytes(&message, sizeof(message));
    
    if (!sendCommand(CMD_SWITCH_CHANNEL_OP, data))
    {
        LOG("failed to switch channel");
        data->release();
        
        return false;
    }
    
    data->release();
    
    return true;
}

bool WirelessOneMT76::initGain(uint32_t index, uint8_t values[], uint32_t length)
{
    OSData *data = OSData::withCapacity(sizeof(index) + length);
    
    data->appendBytes(&index, sizeof(index));
    data->appendBytes(values, length);
    
    if (!sendCommand(CMD_INIT_GAIN_OP, data))
    {
        LOG("failed to init gain");
        data->release();
        
        return false;
    }
    
    data->release();
    
    return true;
}

bool WirelessOneMT76::setLedMode(uint32_t index)
{
    OSData *data = OSData::withCapacity(sizeof(index));
    
    data->appendBytes(&index, sizeof(index));
    
    if (!sendCommand(CMD_LED_MODE_OP, data))
    {
        LOG("failed to set LED mode");
        data->release();
        
        return false;
    }
    
    data->release();
    
    return true;
}

bool WirelessOneMT76::sendCommand(McuCommand command, OSData *data)
{
    // Values must be 32-bit aligned
    // 32 zero-bits mark the end
    uint32_t length = data->getLength();
    uint8_t padding = length % sizeof(uint32_t);
    uint32_t footer = 0;
    
    // We ignore responses, sequence number is always zero
    TxInfoCommand info = {};
    
    info.port = CPU_TX_PORT;
    info.infoType = CMD_PACKET;
    info.commandType = command;
    info.length = length + padding;
    
    OSData *out = OSData::withCapacity(sizeof(info) + info.length + sizeof(footer));
    
    out->appendBytes(&info, sizeof(info));
    out->appendBytes(data);
    out->appendBytes(&footer, padding);
    out->appendBytes(&footer, sizeof(footer));
    
    if (!bulkWrite(out))
    {
        LOG("failed to write command");
        out->release();
        
        return false;
    }
    
    out->release();
    
    return true;
}

uint32_t WirelessOneMT76::efuseRead(uint8_t address, uint8_t index)
{
    EfuseControl control = {};
    
    // Set address to read from
    // Kick-off read
    control.value = controlRead(MT_EFUSE_CTRL);
    control.mode = 0;
    control.addressIn = address;
    control.kick = 1;
    
    controlWrite(MT_EFUSE_CTRL, control.value);
    
    while (controlRead(MT_EFUSE_CTRL) & MT_EFUSE_CTRL_KICK);
    
    return controlRead(MT_EFUSE_DATA(index));
}

bool WirelessOneMT76::writeBeacon(uint32_t offset, uint8_t type, OSData *data)
{
    uint8_t broadcastAddress[] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };
    
    TxWi txWi = {};
    
    // OFDM transmission method
    // Generate beacon timestamp
    // Use hardware sequence control
    txWi.phyMode = MT_PHY_TYPE_OFDM;
    txWi.timestamp = 1;
    txWi.nseq = 1;
    txWi.mpduByteCount = sizeof(WlanFrame) + data->getLength();
    
    WlanFrame wlanFrame = {};
    
    wlanFrame.frameControl.type = MT_WLAN_MGMT;
    wlanFrame.frameControl.subtype = type;
    
    memcpy(wlanFrame.destination, broadcastAddress, sizeof(macAddress));
    memcpy(wlanFrame.source, macAddress, sizeof(macAddress));
    memcpy(wlanFrame.bssId, macAddress, sizeof(macAddress));
    
    OSData *out = OSData::withCapacity(sizeof(txWi) + sizeof(wlanFrame) + data->getLength());
    
    out->appendBytes(&txWi, sizeof(txWi));
    out->appendBytes(&wlanFrame, sizeof(wlanFrame));
    out->appendBytes(data);
    
    BeaconTimeConfig config = {};
    
    // Enable timing synchronization function (TSF) timer
    // Enable target beacon transmission time (TBTT) timer
    // Set TSF timer to AP mode
    // Activate beacon transmission
    config.value = controlRead(MT_BEACON_TIME_CFG);
    config.tsfTimerEnabled = 1;
    config.tbttTimerEnabled = 1;
    config.tsfSyncMode = 3;
    config.transmitBeacon = 1;
    
    if (!burstWrite(MT_BEACON_BASE + offset, (uint8_t*)out->getBytesNoCopy(), out->getLength()))
    {
        LOG("failed to write beacon");
        out->release();
        
        return false;
    }
    
    controlWrite(MT_BEACON_TIME_CFG, config.value);
    
    out->release();
    
    return true;
}

bool WirelessOneMT76::startBulkRead(IOUSBHostPipe *pipe)
{
    uint16_t length = pipe->getEndpointDescriptor()->wMaxPacketSize;
    IOBufferMemoryDescriptor *buffer = IOBufferMemoryDescriptor::withCapacity(length, kIODirectionIn);
    
    if (!buffer)
    {
        LOG("failed to allocate buffer");
        
        return false;
    }
    
    BulkReadInfo *info = (BulkReadInfo*)IOMalloc(sizeof(BulkReadInfo));
    
    if (!info)
    {
        LOG("failed to allocate read info");
        buffer->release();
        
        return false;
    }
    
    info->pipe = pipe;
    info->buffer = buffer;
    
    IOUSBHostCompletion completion = {};
    
    completion.owner = this;
    completion.action = bulkReadCompleted;
    completion.parameter = info;
    
    IOReturn error = pipe->io(buffer, length, &completion);
    
    if (error != kIOReturnSuccess)
    {
        LOG("input error: 0x%.8x", error);
        buffer->release();
        
        return false;
    }
    
    return true;
}

bool WirelessOneMT76::bulkWrite(OSData *data)
{
    IOMemoryDescriptor *buffer = IOMemoryDescriptor::withAddress(
        (uint8_t*)data->getBytesNoCopy(),
        data->getLength(),
        kIODirectionOut
    );
    
    if (!buffer)
    {
        LOG("failed to allocate buffer");
        
        return false;
    }
    
    buffer->prepare();
    
    uint32_t bytesTransferred = 0;
    IOReturn error = writePipe->io(buffer, data->getLength(), bytesTransferred);
    
    if (error != kIOReturnSuccess)
    {
        LOG("output error: 0x%.8x", error);
        
        return false;
    }
    
    buffer->complete();
    buffer->release();
    
    return true;
}

uint32_t WirelessOneMT76::controlRead(uint16_t address, VendorRequest request)
{
    DeviceRequest deviceRequest;
    
    deviceRequest.bmRequestType = makeDeviceRequestbmRequestType(kRequestDirectionIn, kRequestTypeVendor, kRequestRecipientDevice);
    deviceRequest.bRequest = request;
    deviceRequest.wValue = 0;
    deviceRequest.wIndex = address;
    deviceRequest.wLength = sizeof(uint32_t);
    
    uint32_t response = 0;
    uint32_t bytesTransferred = 0;
    IOReturn error = device->deviceRequest(this, deviceRequest, &response, bytesTransferred);
    
    if (error != kIOReturnSuccess)
    {
        LOG("request error: 0x%.8x", error);
        
        return 0;
    }
    
    return response;
}

void WirelessOneMT76::controlWrite(uint16_t address, uint32_t value, VendorRequest request)
{
    DeviceRequest deviceRequest;
    uint32_t bytesTransferred = 0;
    IOReturn error;
    
    deviceRequest.bmRequestType = makeDeviceRequestbmRequestType(kRequestDirectionOut, kRequestTypeVendor, kRequestRecipientDevice);
    deviceRequest.bRequest = request;
    
    if (request == MT_VEND_DEV_MODE)
    {
        deviceRequest.wValue = address;
        deviceRequest.wIndex = 0;
        deviceRequest.wLength = 0;
        
        error = device->deviceRequest(this, deviceRequest, (void*)nullptr, bytesTransferred);
    }
    
    else
    {
        deviceRequest.wValue = 0;
        deviceRequest.wIndex = address;
        deviceRequest.wLength = sizeof(uint32_t);
        
        error = device->deviceRequest(this, deviceRequest, &value, bytesTransferred);
    }
    
    if (error != kIOReturnSuccess)
    {
        LOG("request error: 0x%.8x", error);
    }
}

void WirelessOneMT76::timerAction(OSObject *owner, IOTimerEventSource *sender)
{
    timeout = true;
}

void WirelessOneMT76::requestResourceCallback(
    OSKextRequestTag requestTag,
    OSReturn result,
    const void *resourceData,
    uint32_t resourceDataLength,
    void *context
) {
    WirelessOneMT76 *self = (WirelessOneMT76*)context;
    
    IOLockLock(self->resourceLock);
    
    if (result == kOSReturnSuccess)
    {
        self->firmware = OSData::withBytes(resourceData, resourceDataLength);
    }
    
    else
    {
        LOG("error requesting firmware: 0x%.8x", result);
    }
    
    IOLockUnlock(self->resourceLock);
    IOLockWakeup(self->resourceLock, self, true);
}

void WirelessOneMT76::bulkReadCompleted(
    void *owner,
    void *parameter,
    IOReturn status,
    uint32_t bytesTransferred
) {
    WirelessOneMT76 *self = (WirelessOneMT76*)owner;
    BulkReadInfo *info = (BulkReadInfo*)parameter;
    uint8_t *data = (uint8_t*)info->buffer->getBytesNoCopy();
    uint32_t length = (uint32_t)info->buffer->getLength();
    
    if (status != kIOReturnSuccess)
    {
        LOG("input error: 0x%.8x", status);
    }
    
    else if (length < sizeof(RxInfoCommand))
    {
        LOG("invalid data");
    }
    
    else if (info->pipe == self->readPipe)
    {
        RxInfoCommand *rxInfo = (RxInfoCommand*)data;
        
        if (rxInfo->eventType == EVT_PACKET_RX)
        {
            // Skip RxWi data
            self->handleControllerPacket(data + sizeof(RxInfoCommand) + sizeof(RxWi));
        }
        
        else if (rxInfo->eventType == EVT_BUTTON_PRESS)
        {
            // Start sending the 'pairing' beacon
            self->writeBeacon(MT_BCN_PAIR_OFFSET, MT_WLAN_PAIR, nullptr);
            
            LOG("pairing beacon started");
        }
    }
    
    else if (info->pipe == self->packetPipe)
    {
        RxInfoPacket *rxInfo = (RxInfoPacket*)data;
        
        if (rxInfo->is80211)
        {
            // Skip RxWi data
            self->handleWlanPacket(data + sizeof(RxInfoPacket) + sizeof(RxWi));
        }
    }
    
    // Start next read
    self->startBulkRead(info->pipe);
    
    IOFree(info, sizeof(BulkReadInfo));
}

uint32_t WirelessOneMT76::getLocationId()
{
    OSNumber *id = OSDynamicCast(OSNumber, device->getProperty(kUSBHostPropertyLocationID));
    
    if (!id)
    {
        LOG("no location id found");
        
        return 0;
    }
    
    return id->unsigned32BitValue();
}

void WirelessOneMT76::dispose()
{
    LOG("releasing resources");
    
    if (dongle)
    {
        dongle->terminate(kIOServiceRequired);
        dongle->detachAll(gIOServicePlane);
        dongle->release();
        dongle = nullptr;
    }
    
    if (firmware)
    {
        firmware->release();
        firmware = nullptr;
    }
    
    if (resourceLock)
    {
        IOLockFree(resourceLock);
        resourceLock = nullptr;
    }
    
    if (timer)
    {
        timer->cancelTimeout();
        workLoop->removeEventSource(timer);
        timer->release();
        timer = nullptr;
    }
    
    if (readPipe)
    {
        readPipe->abort();
        readPipe->release();
        readPipe = nullptr;
    }
    
    if (writePipe)
    {
        writePipe->abort();
        writePipe->release();
        writePipe = nullptr;
    }
    
    if (packetPipe)
    {
        packetPipe->abort();
        packetPipe->release();
        packetPipe = nullptr;
    }
    
    if (interface)
    {
        interface->close(this);
        interface = nullptr;
    }
    
    if (device)
    {
        device->close(this);
        device = nullptr;
    }
}

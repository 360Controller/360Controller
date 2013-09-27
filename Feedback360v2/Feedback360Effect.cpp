#include "Feedback360Effect.h"

//----------------------------------------------------------------------------------------------
// CEffect
//----------------------------------------------------------------------------------------------
Feedback360Effect::Feedback360Effect()
{
    Type  = NULL;
    memset(&DiEffect, 0, sizeof(FFEFFECT));
    memset(&DiEnvelope, 0, sizeof(FFENVELOPE));
    memset(&DiConstantForce, 0, sizeof(FFCONSTANTFORCE));
    memset(&DiPeriodic, 0, sizeof(FFPERIODIC));
    memset(&DiRampforce, 0, sizeof(FFRAMPFORCE));

    Status  = 0;
    PlayCount = 0;
    StartTime = 0;
    Index = 0;
}

//----------------------------------------------------------------------------------------------
// Calc
//----------------------------------------------------------------------------------------------
void Feedback360Effect::Calc(LONG *LeftLevel, LONG *RightLevel)
{
    // エフェクトの再生時間、開始時刻、終了時刻、現在時刻を算出する
    CFTimeInterval Duration = NULL;
    if(DiEffect.dwDuration != FF_INFINITE) {
       Duration = MAX( 1, DiEffect.dwDuration / 1000 ) / 1000;
    } else {
        Duration = DBL_MAX;
    }
    CFAbsoluteTime BeginTime = StartTime + ( DiEffect.dwStartDelay / (1000*1000) );
    CFAbsoluteTime EndTime  = DBL_MAX;
    if( PlayCount != -1 )
    {
        EndTime = BeginTime + Duration * PlayCount;
    }
    CFAbsoluteTime CurrentTime = CFAbsoluteTimeGetCurrent();

    // エフェクトは再生中？
    if( Status == FFEGES_PLAYING && BeginTime <= CurrentTime && CurrentTime <= EndTime )
    {
        // フォースを計算する
        LONG NormalLevel;
        LONG WorkLeftLevel;
        LONG WorkRightLevel;
        /*
        if(CFEqual(Type, kFFEffectType_CustomForce_ID)) {
            LONG Period	= MAX( 1, ( DiCustomForce.dwSamplePeriod / 1000 ) );
            if(Index >= DiCustomForce.cSamples) {
                Index = 0;
                WorkLeftLevel = 0;
                WorkRightLevel = 0;
            }
            else {
                WorkLeftLevel = DiCustomForce.rglForceData[2*Index];
                WorkRightLevel = DiCustomForce.rglForceData[(2*Index)+1];
                Index++;
            }
        }
        else {
         */
            // エンベロープを計算する
            LONG NormalRate;
            LONG AttackLevel;
            LONG FadeLevel;

            CalcEnvelope(
                         (ULONG)(Duration*1000)
                         ,(ULONG)(fmod(CurrentTime - BeginTime, Duration)*1000)
                         ,&NormalRate
                         ,&AttackLevel
                         ,&FadeLevel );

            CalcForce(
                      (ULONG)(Duration*1000)
                      ,(ULONG)(fmod(CurrentTime - BeginTime, Duration)*1000)
                      ,NormalRate
                      ,AttackLevel
                      ,FadeLevel
                      ,&NormalLevel );

            //fprintf(stderr, "DeltaT %f\n", CurrentTime - BeginTime);
            //fprintf(stderr, "Duration %f; NormalRate: %d; AttackLevel: %d; FadeLevel: %d\n", Duration, NormalRate, AttackLevel, FadeLevel);

            // フォースの正負を調整する
            fprintf(stderr, "NL: %u\n", NormalLevel);
            WorkLeftLevel = (NormalLevel > 0) ? NormalLevel : -NormalLevel;
            WorkRightLevel = (NormalLevel > 0) ? NormalLevel : -NormalLevel;

            // フォースの上限を調整する
            WorkLeftLevel = MIN( SCALE_MAX, WorkLeftLevel * SCALE_MAX / 10000 );
            WorkRightLevel = MIN( SCALE_MAX, WorkRightLevel * SCALE_MAX / 10000 );
        //}
        // フォースを加算する
        *LeftLevel = *LeftLevel + WorkLeftLevel;
        *RightLevel = *RightLevel + WorkRightLevel;
      }/*
      else {
      *LeftLevel = 0;
      *RightLevel = 0;
      }*/
}

//----------------------------------------------------------------------------------------------
// CalcEnvelope
//----------------------------------------------------------------------------------------------
void Feedback360Effect::CalcEnvelope(ULONG Duration, ULONG CurrentPos, LONG *NormalRate, LONG *AttackLevel, LONG *FadeLevel)
{
	//	エンベロープを計算する
	if( ( DiEffect.dwFlags & FFEP_ENVELOPE ) && DiEffect.lpEnvelope != NULL )
	{
		//	アタックの割合を算出する
		LONG	AttackRate	= 0;
		ULONG	AttackTime	= MAX( 1, DiEnvelope.dwAttackTime / 1000 );
		if( CurrentPos < AttackTime )
		{
			AttackRate	= ( AttackTime - CurrentPos ) * 100 / AttackTime;
		}
		//	フェードの割合を算出する
		LONG	FadeRate	= 0;
		ULONG	FadeTime	= MAX( 1, DiEnvelope.dwFadeTime / 1000 );
		ULONG	FadePos		= Duration - FadeTime;
		if( FadePos < CurrentPos )
		{
			FadeRate	= ( CurrentPos - FadePos ) * 100 / FadeTime;
		}
		//	算出した値を返す
		*NormalRate		= 100 - AttackRate - FadeRate;
		*AttackLevel	= DiEnvelope.dwAttackLevel * AttackRate;
		*FadeLevel		= DiEnvelope.dwFadeLevel * FadeRate;
	} else {
		*NormalRate		= 100;
		*AttackLevel	= 0;
		*FadeLevel		= 0;
	}
}

void Feedback360Effect::CalcForce(ULONG Duration, ULONG CurrentPos, LONG NormalRate, LONG AttackLevel, LONG FadeLevel, LONG * NormalLevel)
{
	//	変数宣言
    LONG Magnitude = 0;
    LONG Period;
    LONG R;
    LONG Rate;

	//	エフェクトの種類によって処理を振り分ける
    //	条件
    //	カスタム フォース
    //	コンスタント フォース
    if(CFEqual(Type, kFFEffectType_ConstantForce_ID)) {
        Magnitude	= DiConstantForce.lMagnitude;
        Magnitude	= ( Magnitude * NormalRate + AttackLevel + FadeLevel ) / 100;
    }
    //	周期的エフェクト
    else if(CFEqual(Type, kFFEffectType_Square_ID)) {
        //	１周期の時間（ミリ秒）と経過時間から 0 〜 359 度のどこかを求める
        Period	= MAX( 1, ( DiPeriodic.dwPeriod / 1000 ) );
        R		= ( CurrentPos%Period) * 360 / Period;
        //	フェーズを計算する
        R	= ( R + ( DiPeriodic.dwPhase / 100 ) ) % 360;
        //	エンベロープを考慮したマグニチュードを求める
        Magnitude	= DiPeriodic.dwMagnitude;
        Magnitude	= ( Magnitude * NormalRate + AttackLevel + FadeLevel ) / 100;
        //	正方形を考慮したマグニチュードを求める
        if( 180 <= R )
        {
            Magnitude	= Magnitude * -1;
        }
        //	オフセットを計算する
        Magnitude	= Magnitude + DiPeriodic.lOffset;
    }

    else if(CFEqual(Type, kFFEffectType_Sine_ID)) {
        //	１周期の時間（ミリ秒）と経過時間から 0 〜 359 度のどこかを求める
        Period	= MAX( 1, ( DiPeriodic.dwPeriod / 1000 ) );
        R		= (CurrentPos%Period) * 360 / Period;
        //	フェーズを計算する
        R		= ( R + ( DiPeriodic.dwPhase / 100 ) ) % 360;
        //	エンベロープを考慮したマグニチュードを求める
        Magnitude	= DiPeriodic.dwMagnitude;
        Magnitude	= ( Magnitude * NormalRate + AttackLevel + FadeLevel ) / 100;
        //	正弦波を考慮したマグニチュードを求める
        Magnitude	= ( int)( Magnitude * sin( R * 3.1415 / 180.0 ) );
        //	オフセットを計算する
        Magnitude	= Magnitude + DiPeriodic.lOffset;
    }

    else if(CFEqual(Type, kFFEffectType_Triangle_ID)) {
        //	１周期の時間（ミリ秒）と経過時間から 0 〜 359 度のどこかを求める
        Period	= MAX( 1, ( DiPeriodic.dwPeriod / 1000 ) );
        R		= (CurrentPos%Period) * 360 / Period;
        //	フェーズを計算する
        R		= ( R + ( DiPeriodic.dwPhase / 100 ) ) % 360;
        //	エンベロープを考慮したマグニチュードを求める
        Magnitude	= DiPeriodic.dwMagnitude;
        Magnitude	= ( Magnitude * NormalRate + AttackLevel + FadeLevel ) / 100;
        //	三角波を考慮したマグニチュードを求める
        if( 0 <= R && R < 90 )
        {
            Magnitude	= -Magnitude * ( 90 - R ) / 90;
        }
        if( 90 <= R && R < 180 )
        {
            Magnitude	= Magnitude * ( R - 90 ) / 90;
        }
        if( 180 <= R && R < 270 )
        {
            Magnitude	= Magnitude * ( 90 - ( R - 180 ) ) / 90;
        }
        if( 270 <= R && R < 360 )
        {
            Magnitude	= -Magnitude * ( R - 270 ) / 90;
        }
        //	オフセットを計算する
        Magnitude	= Magnitude + DiPeriodic.lOffset;
    }
    else if(CFEqual(Type, kFFEffectType_SawtoothUp_ID)) {
        //	１周期の時間（ミリ秒）と経過時間から 0 〜 359 度のどこかを求める
        Period	= MAX( 1, ( DiPeriodic.dwPeriod / 1000 ) );
        R		= (CurrentPos%Period) * 360 / Period;
        //	フェーズを計算する
        R		= ( R + ( DiPeriodic.dwPhase / 100 ) ) % 360;
        //	エンベロープを考慮したマグニチュードを求める
        Magnitude	= DiPeriodic.dwMagnitude;
        Magnitude	= ( Magnitude * NormalRate + AttackLevel + FadeLevel ) / 100;
        //	アップ鋸歯を考慮したマグニチュードを求める
        if( 0 <= R && R < 180 )
        {
            Magnitude	= -Magnitude * ( 180 - R ) / 180;
        }
        if( 180 <= R && R < 360 )
        {
            Magnitude	= Magnitude * ( R - 180 ) / 180;
        }
        //	オフセットを計算する
        Magnitude	= Magnitude + DiPeriodic.lOffset;
    }
    else if(CFEqual(Type, kFFEffectType_SawtoothDown_ID)) {
        //	１周期の時間（ミリ秒）と経過時間から 0 〜 359 度のどこかを求める
        Period	= MAX( 1, ( DiPeriodic.dwPeriod / 1000 ) );
        R		= (CurrentPos%Period) * 360 / Period;
        //	フェーズを計算する
        R		= ( R + ( DiPeriodic.dwPhase / 100 ) ) % 360;
        //	エンベロープを考慮したマグニチュードを求める
        Magnitude	= DiPeriodic.dwMagnitude;
        Magnitude	= ( Magnitude * NormalRate + AttackLevel + FadeLevel ) / 100;
        //	ダウン鋸歯を考慮したマグニチュードを求める
        if( 0 <= R && R < 180 )
        {
            Magnitude	= Magnitude * ( 180 - R ) / 180;
        }
        if( 180 <= R && R < 360 )
        {
            Magnitude	= -Magnitude * ( R - 180 ) / 180;
        }
        //	オフセットを計算する
        Magnitude	= Magnitude + DiPeriodic.lOffset;
    }

    // 傾斜フォース
    else if(CFEqual(Type, kFFEffectType_RampForce_ID)) {
        //	始点、終点の割合を算出する
        Rate		= ( Duration - CurrentPos ) * 100
        / Duration;//MAX( 1, DiEffect.dwDuration / 1000 );
        //	エンベロープを考慮したマグニチュードを求める
        Magnitude	= ( DiRampforce.lStart * Rate
                       + DiRampforce.lEnd * ( 100 - Rate ) ) / 100;
        Magnitude	= ( Magnitude * NormalRate + AttackLevel + FadeLevel ) / 100;
    }
    // ゲインを考慮したフォースを返す
    *NormalLevel = Magnitude * (LONG)DiEffect.dwGain / 10000;
}

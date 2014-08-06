////////////////////////////////////////////////////////////////////////////////
//メカトロニクス実習 装置制御ソースコード                                     //
//Version : 1.0最終 (2013.9.27)                                               //
//Author  : 庄子裕介, 林修煌                                                  //
////////////////////////////////////////////////////////////////////////////////
//マイコン ポート設定                                                         //
//アームモータ    : P1 B2=CW, B3=CCW                                          //
//ウェーハモータ  : P2 B2=CW, B3=CCW                                          //
//LED             : PB                                                        //
//スイッチボタン  : PA                                                        //
//ラインセンサ    : P7-0(AN0)                                                 //
//ソレノイド      : P4 B0                                                     //
//オートスイッチ  : P5 B0(上), B1(下)                                         //
////////////////////////////////////////////////////////////////////////////////
//ステップ分割数設定 1パルス 0.225度                                          //
////////////////////////////////////////////////////////////////////////////////

/*  ヘッダインクルード  */
#include       <3052.h>                                                          // 3052Fの定義ヘッダ
#include       <stdio.h>
#include       <math.h>

/*  パラメータ設定  */
#define        DEV 1600                                                          //モータ分解能(ステップ角0.225°なら1600)
#define        MODIFY_END 5                                                      //修正完了確認値（偏芯距離がこの値より小さければ修正が完了したものとする）

/*  プロトタイプ宣言  */
int     ADConvert(void);
void    WaitStart(void);
void    FindNotch(void);
void    FindError(void);
void    Measurement(void);
void    ModifyError(void);
void    SetArmMotor(void);
void    InitialSetting(void);
void    Wait(int type, int waitTime);
void    ControlCylinder(int function);
void    DriveArmMotor(long int setPosition);
void    DriveWaferMotor(int rotationDirection, int pulse);

/*  グローバル変数宣言  */
long int     PositionArmMotor;                                                   //アームモータ現在位置(パルス数)
long int     ArmMotorDef = 30000;                                                //ウェーハをとりに行くときのパルス数
int     MeasurementData[DEV];                                                    //測定データ格納配列
int     ErrorDistance;                                                           /*偏芯量（中心からのずれ長さ）
                                                                                   (-511～482までのデジタル値) 後でアームが1パルスで何mm進むか調べて変換する（mm→パルス）*/
int     ErrorDegree;                                                             //偏芯量（角度）(パルス数)
int     NotchDegree;                                                             //ノッチの角度(パルス数)
int     NotchDetectionFlag;                                                      //ノッチ検出フラグ(0:失敗, 1:成功)

////////////////////////////////////////////////////////////////////////////////
//Name     : main                                                             //
//Details  : メイン関数                                                       //
//Argument : -                                                                //
//Return   : -                                                                //
////////////////////////////////////////////////////////////////////////////////
int     main(void)
{
    //TestMain();                                                                  //試験のときはこれを呼ぶ

    /* 初期化処理*/
    InitialSetting();                                                            //初期設定
    ControlCylinder(0);                                                          //シリンダを初期位置（下降位置）に設定
    SetArmMotor();                                                               //アームを適切な位置まで移動(手動)
    Wait(0,50);
    ControlCylinder(1);                                                          //シリンダ上昇

    /*ユーザがアーム上へウェーハをセット*/

    WaitStart();                                                                 //スイッチが押されるまで待つ

    /*ウェーハをペデスタル上まで運ぶ*/
    while(1)
    {
         DriveArmMotor(PositionArmMotor+1);

        if(ADConvert() < 482)
        {
            break;
        }
    }

    PositionArmMotor = 0;

    Wait(0,50);
    ControlCylinder(0);                                                          //シリンダを下げる
    Wait(0,50);
    Measurement();                                                               //偏芯測定開始
    Wait(0,50);

    FindNotch();                                                                 //ノッチ検出
    FindError();                                                                 //偏芯検出演算
    ModifyError();                                                               //偏芯を修正する

    Wait(0,100);

    Measurement();

    /*偏芯が修正されるまで繰り返す*/
    /*
    do
    {
        ModifyError();                                                           //偏芯を修正する
        Measurement();
        FindNotch();                                                             //ノッチ検出
        FindError();                                                             //偏芯検出演算
    }while (ErrorDistance > MODIFY_END);

    DriveArmMotor(0);                                                            //アームモータを初期位置へ戻す
    */

    return 0;
}

////////////////////////////////////////////////////////////////////////////////
//Name     : InitialSetting                                                   //
//Details  : 初期化                                                           //
//Argument : -                                                                //
//Return   : -                                                                //
////////////////////////////////////////////////////////////////////////////////
void    InitialSetting(void)
{
    /*マイコン ポート設定*/
    P1.DDR = 0xff;                                                               // P1(アームモータ)を出力ポートに設定
    P2.DDR = 0xff;                                                               // P2(ウェーハモータ)を出力ポートに設定
    P3.DDR = 0x00;                                                               // P3(フォトセンサ)を入力ポートに設定
    P4.DDR = 0xff;                                                               // P4（ソレノイド）を出力ポートに設定
    P5.DDR = 0x00;                                                               // P5(オートスイッチ)を入力ポートに設定
    PA.DDR = 0x00;                                                               // PA(スイッチボタン)を入力ポートに設定
    PB.DDR = 0xff;                                                               // PB(LED)を出力ポートに設定
    PB.DR.BYTE = 0xff;                                                           // PB(LED)を全て消灯

    return 0;
}

////////////////////////////////////////////////////////////////////////////////
//Name     : SetArmMotor                                                      //
//Details  : アームモータ自由移動                                             //
//Argument : -                                                                //
//Return   : -                                                                //
////////////////////////////////////////////////////////////////////////////////
void    SetArmMotor(void)
{
    while(-1)
    {
        PB.DR.BYTE = 0x00;

        if(!PA.DR.BIT.B0)
        {
            DriveArmMotor(PositionArmMotor-1);
        }

        else if(!PA.DR.BIT.B1)
        {
            DriveArmMotor(PositionArmMotor+1);
        }

        else if(!PA.DR.BIT.B2)
        {
            PositionArmMotor = 0;
            break;
        }

    }
}

////////////////////////////////////////////////////////////////////////////////
//Name     : Wait                                                             //
//Details  : 待ち時間発生                                                     //
//Argument : type             待ち時間タイプ                                  //
//           waitTime         時間指定                                        //
//Return   : -                                                                //
////////////////////////////////////////////////////////////////////////////////
//type=0 秒単位指定 waitTime=100で約1秒待つ                                   //
//type=1 アームモータ回転ディレイ                                             //
//type=2 ウェーハモータ回転ディレイ                                           //
////////////////////////////////////////////////////////////////////////////////
void    Wait(int type, int waitTime)
{
    int     t;                                                                   //内部変数tを定義
    int     i;                                                                   //内部変数iを定義

    switch(type)
    {
        case 0 :
            t = 8000;
            break;
        case 1 :
            t = 10;
            break;
        case 2 :
            t = 100;
            break;
        default :
            t = 8000;
    }

    while(waitTime--)
    {
        i = t;
        while(i--){}
    }

    return 0;
}

////////////////////////////////////////////////////////////////////////////////
//Name     : DriveArmMotor                                                    //
//Details  : アームモータ駆動                                                 //
//Argument : setPosition               アームの絶対位置指定                   //
//Return   : -                                                                //
////////////////////////////////////////////////////////////////////////////////
void    DriveArmMotor(long int setPosition)
{
    P1.DR.BYTE = 0x00;                                                            //マイコンポート初期化

    if (setPosition > PositionArmMotor)
    {
        while (setPosition != PositionArmMotor)
        {
            P1.DR.BIT.B3 = 1;
            Wait(1,4);
            P1.DR.BIT.B3 = 0;
            Wait(1,4);

            PositionArmMotor++;
        }
    }

    else if (setPosition < PositionArmMotor)
    {
        while (setPosition != PositionArmMotor)
        {
            P1.DR.BIT.B2 = 1;
            Wait(1,4);
            P1.DR.BIT.B2 = 0;
            Wait(1,4);

            PositionArmMotor--;
        }
    }

        return 0;
}

////////////////////////////////////////////////////////////////////////////////
//Name     : DriveWaferMotor                                                  //
//Details  : ウェーハモータ駆動                                               //
//Argument : rotationDirection          回転方向 [0:CW, 1:CCW]                //
//           pulse                      パルス数                              //
//Return   : -                                                                //
////////////////////////////////////////////////////////////////////////////////
void    DriveWaferMotor(int rotationDirection, int pulse)
{
    P2.DR.BYTE =0x00;                                                            //マイコンポート初期化

    /*時計回り(CW)*/
    if (rotationDirection == 0)
    {
        while (pulse--)
        {
            P2.DR.BIT.B3 = 1;
            Wait(2,20);
            P2.DR.BIT.B3 = 0;
            Wait(2,20);
        }
    }

    /*反時計回り(CCW)*/
    else
    {
        while (pulse--)
        {
            P2.DR.BIT.B2 = 1;
            Wait(2,20);
            P2.DR.BIT.B2 = 0;
            Wait(2,20);
        }
    }

    return 0;
}

////////////////////////////////////////////////////////////////////////////////
//Name     : ADConvert                                                        //
//Details  : ADコンバータ                                                     //
//Argument : -                                                                //
//Return   : temp                       変換後データ                          //
////////////////////////////////////////////////////////////////////////////////

int     ADConvert(void)
{
    int     temp;                                                                //変換データ格納変数

    AD.ADCSR.BIT.ADF = 0;                                                        //ADFフラグクリア
    AD.ADCSR.BIT.SCAN = 0;                                                       //単一モード
    AD.ADCSR.BIT.CKS = 1;                                                        //クロックセレクト 134モード
    AD.ADCSR.BIT.CH = 0;                                                         //チャンネル AN0単一モード(P128参照)

    AD.ADCSR.BIT.ADST = 1;                                                       //AD変換開始

    while (AD.ADCSR.BIT.ADF == 0);                                               //変換終了まで待つ

    temp = AD.ADDRA >> 6;                                                        //右に6BITシフト(P127参照)大事！

    return (temp);
}

////////////////////////////////////////////////////////////////////////////////
//Name     : ControlCylinder                                                  //
//Details  : シリンダ制御                                                     //
//Argument : function                   動作設定[0:下降, 1:上昇]              //
//Return   : -                                                                //
////////////////////////////////////////////////////////////////////////////////
void    ControlCylinder(int function)
{
    if (function == 0)
    {
        P4.DR.BIT.B0 = 0;

        while(P5.DR.BIT.B0)
        {
        }
    }

    else
    {
        P4.DR.BIT.B0 = 1;

        while(P5.DR.BIT.B1)
        {
        }
    }

    return 0;
}

////////////////////////////////////////////////////////////////////////////////
//Name     : Measurement                                                      //
//Details  : 偏芯測定                                                         //
//Argument : -                                                                //
//Return   : -                                                                //
////////////////////////////////////////////////////////////////////////////////
void    Measurement(void)
{
    int     cnt = 0;                                                             //カウンタ

    for (cnt = 0; cnt < DEV; cnt++)
    {
        MeasurementData[cnt] = ADConvert();                                      //データをデジタル値(0 - 1023)へコンバート
        DriveWaferMotor(0,1);                                                    //ウェーハモータを1パルス回転
    }


    return 0;
}

////////////////////////////////////////////////////////////////////////////////
//Name     : WaitStart                                                        //
//Details  : 処理開始待ち（スイッチ押下）                                     //
//Argument : -                                                                //
//Return   : -                                                                //
////////////////////////////////////////////////////////////////////////////////
void    WaitStart(void)
{
    while (-1)
    {
        if(!PA.DR.BIT.B0)
        {
            break;
        }
    }
    return 0;
}

////////////////////////////////////////////////////////////////////////////////
//Name     : FindNotch                                                        //
//Details  : ノッチ検出                                                       //
//Argument : -                                                                //
//Return   : -                                                                //
////////////////////////////////////////////////////////////////////////////////
void    FindNotch(void)
{
    int     cnt;                                                                 //カウンタ
    int     flag = 0;                                                            //検出終了フラグ
    int     max = 0;                                                             //一時値格納変数
    int     test;                                                                //内部変数

    /*ノッチを見つける*/
    for (cnt = 0; cnt < (DEV-2); cnt++)
    {
        test = MeasurementData[cnt+2] - MeasurementData[cnt];                    //偏芯の変位を代入


        /*閾値よりも大きな変化であればノッチ検出開始*/
        if (fabs(test) > max)
        {
            max = fabs(test);
            NotchDegree = cnt;                                                   //ノッチの角度(パルス数)を保存
            NotchDetectionFlag = 1;                                              //ノッチ検出フラグをONに

         }

        /*検出できなかった場合*/

        if (cnt == (DEV - 2))
        {
            NotchDetectionFlag = 0;                                              //ノッチ検出フラグをOFFに
            break;
        }

    }
    return 0;
}

////////////////////////////////////////////////////////////////////////////////
//Name     : FindError                                                        //
//Details  : 偏芯検出                                                         //
//Argument : -                                                                //
//Return   : -                                                                //
////////////////////////////////////////////////////////////////////////////////
void    FindError(void)
{
    int     cnt;                                                                 //カウンタ
    int     max;                                                                 //最大値格納変数
    int     normalizeData[DEV];                                                  //正規化データ格納配列

    /*配列を正規化して絶対値を取る*/
    for (cnt = 0; cnt <DEV; cnt++)
    {
        normalizeData[cnt] = fabs(MeasurementData[cnt] - 482);                   //絶対値を取る
    }

    /*偏芯検出開始*/
    if (NotchDegree < (DEV/2))                                                   //ノッチが0～πまでのとき
    {
        max = normalizeData[800];

        for (cnt = DEV/2; cnt < DEV; cnt++)
        {
            /*偏芯が最大となる角度を見つける*/
            if (max < normalizeData[cnt])
            {
                max = normalizeData[cnt];
                ErrorDegree = cnt;                                               //偏芯角度をグローバル変数へ代入
                ErrorDistance = MeasurementData[cnt] - 482;                      //偏芯距離をグローバル変数へ代入
            }
        }
    }

    else                                                                         //ノッチがπ～2πまでのとき
    {
        max = normalizeData[0];

        for (cnt = 0; cnt < (DEV/2); cnt++)
        {
            /*偏芯が最大となる角度を見つける*/
            if (max < normalizeData[cnt])
            {
                max = normalizeData[cnt];
                ErrorDegree = cnt;                                               //偏芯角度をグローバル変数へ代入
                ErrorDistance = MeasurementData[cnt] - 482;                      //偏芯距離をグローバル変数へ代入
            }
        }
    }

    return 0;
}

////////////////////////////////////////////////////////////////////////////////
//Name     : ModifyError                                                      //
//Details  : 偏芯修正                                                         //
//Argument : -                                                                //
//Return   : -                                                                //
////////////////////////////////////////////////////////////////////////////////
void    ModifyError(void)
{

    long int     distancePulse;                                                  //修正距離(パルス数)
    int     cnt;                                                                 //カウンタ

    /*偏芯距離換算（センサ値→パルス数）*/
    distancePulse = (ErrorDistance * 7.8125) ;      //修正パルス数算出

    /*偏芯角度修正*/
    DriveWaferMotor(0, ErrorDegree);

    Wait(0,100);

    /*偏芯距離修正*/
    ControlCylinder(1);                                                          //シリンダを上げる
    Wait(0,50);
    DriveArmMotor(distancePulse);                                                //距離修正
    Wait(0,50);
    ControlCylinder(0);                                                          //シリンダを下げる
    Wait(0,50);
    Wait(0,100);

     /*ノッチの位置を合わせる*/
     if (NotchDetectionFlag == 1)
     {
        if (ErrorDegree > NotchDegree)
        {
            DriveWaferMotor(1, (ErrorDegree - NotchDegree));
        }

        else
        {
            DriveWaferMotor(0, (NotchDegree - ErrorDegree));
        }
    }

    /*測定データ格納配列初期化*/
    for (cnt = 0; cnt<DEV; cnt++)
    {
        MeasurementData[cnt] = 0;
    }
    /*グローバル変数を初期化*/
    ErrorDegree = 0;
    ErrorDistance = 0;

    return 0;
}

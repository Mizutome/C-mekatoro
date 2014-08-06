////////////////////////////////////////////////////////////////////////////////
//���J�g���j�N�X���K ���u����\�[�X�R�[�h                                     //
//Version : 1.0�ŏI (2013.9.27)                                               //
//Author  : ���q�T��, �яC��                                                  //
////////////////////////////////////////////////////////////////////////////////
//�}�C�R�� �|�[�g�ݒ�                                                         //
//�A�[�����[�^    : P1 B2=CW, B3=CCW                                          //
//�E�F�[�n���[�^  : P2 B2=CW, B3=CCW                                          //
//LED             : PB                                                        //
//�X�C�b�`�{�^��  : PA                                                        //
//���C���Z���T    : P7-0(AN0)                                                 //
//�\���m�C�h      : P4 B0                                                     //
//�I�[�g�X�C�b�`  : P5 B0(��), B1(��)                                         //
////////////////////////////////////////////////////////////////////////////////
//�X�e�b�v�������ݒ� 1�p���X 0.225�x                                          //
////////////////////////////////////////////////////////////////////////////////

/*  �w�b�_�C���N���[�h  */
#include       <3052.h>                                                          // 3052F�̒�`�w�b�_
#include       <stdio.h>
#include       <math.h>

/*  �p�����[�^�ݒ�  */
#define        DEV 1600                                                          //���[�^����\(�X�e�b�v�p0.225���Ȃ�1600)
#define        MODIFY_END 5                                                      //�C�������m�F�l�i�ΐc���������̒l��菬������ΏC���������������̂Ƃ���j

/*  �v���g�^�C�v�錾  */
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

/*  �O���[�o���ϐ��錾  */
long int     PositionArmMotor;                                                   //�A�[�����[�^���݈ʒu(�p���X��)
long int     ArmMotorDef = 30000;                                                //�E�F�[�n���Ƃ�ɍs���Ƃ��̃p���X��
int     MeasurementData[DEV];                                                    //����f�[�^�i�[�z��
int     ErrorDistance;                                                           /*�ΐc�ʁi���S����̂��꒷���j
                                                                                   (-511�`482�܂ł̃f�W�^���l) ��ŃA�[����1�p���X�ŉ�mm�i�ނ����ׂĕϊ�����imm���p���X�j*/
int     ErrorDegree;                                                             //�ΐc�ʁi�p�x�j(�p���X��)
int     NotchDegree;                                                             //�m�b�`�̊p�x(�p���X��)
int     NotchDetectionFlag;                                                      //�m�b�`���o�t���O(0:���s, 1:����)

////////////////////////////////////////////////////////////////////////////////
//Name     : main                                                             //
//Details  : ���C���֐�                                                       //
//Argument : -                                                                //
//Return   : -                                                                //
////////////////////////////////////////////////////////////////////////////////
int     main(void)
{
    //TestMain();                                                                  //�����̂Ƃ��͂�����Ă�

    /* ����������*/
    InitialSetting();                                                            //�����ݒ�
    ControlCylinder(0);                                                          //�V�����_�������ʒu�i���~�ʒu�j�ɐݒ�
    SetArmMotor();                                                               //�A�[����K�؂Ȉʒu�܂ňړ�(�蓮)
    Wait(0,50);
    ControlCylinder(1);                                                          //�V�����_�㏸

    /*���[�U���A�[����փE�F�[�n���Z�b�g*/

    WaitStart();                                                                 //�X�C�b�`���������܂ő҂�

    /*�E�F�[�n���y�f�X�^����܂ŉ^��*/
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
    ControlCylinder(0);                                                          //�V�����_��������
    Wait(0,50);
    Measurement();                                                               //�ΐc����J�n
    Wait(0,50);

    FindNotch();                                                                 //�m�b�`���o
    FindError();                                                                 //�ΐc���o���Z
    ModifyError();                                                               //�ΐc���C������

    Wait(0,100);

    Measurement();

    /*�ΐc���C�������܂ŌJ��Ԃ�*/
    /*
    do
    {
        ModifyError();                                                           //�ΐc���C������
        Measurement();
        FindNotch();                                                             //�m�b�`���o
        FindError();                                                             //�ΐc���o���Z
    }while (ErrorDistance > MODIFY_END);

    DriveArmMotor(0);                                                            //�A�[�����[�^�������ʒu�֖߂�
    */

    return 0;
}

////////////////////////////////////////////////////////////////////////////////
//Name     : InitialSetting                                                   //
//Details  : ������                                                           //
//Argument : -                                                                //
//Return   : -                                                                //
////////////////////////////////////////////////////////////////////////////////
void    InitialSetting(void)
{
    /*�}�C�R�� �|�[�g�ݒ�*/
    P1.DDR = 0xff;                                                               // P1(�A�[�����[�^)���o�̓|�[�g�ɐݒ�
    P2.DDR = 0xff;                                                               // P2(�E�F�[�n���[�^)���o�̓|�[�g�ɐݒ�
    P3.DDR = 0x00;                                                               // P3(�t�H�g�Z���T)����̓|�[�g�ɐݒ�
    P4.DDR = 0xff;                                                               // P4�i�\���m�C�h�j���o�̓|�[�g�ɐݒ�
    P5.DDR = 0x00;                                                               // P5(�I�[�g�X�C�b�`)����̓|�[�g�ɐݒ�
    PA.DDR = 0x00;                                                               // PA(�X�C�b�`�{�^��)����̓|�[�g�ɐݒ�
    PB.DDR = 0xff;                                                               // PB(LED)���o�̓|�[�g�ɐݒ�
    PB.DR.BYTE = 0xff;                                                           // PB(LED)��S�ď���

    return 0;
}

////////////////////////////////////////////////////////////////////////////////
//Name     : SetArmMotor                                                      //
//Details  : �A�[�����[�^���R�ړ�                                             //
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
//Details  : �҂����Ԕ���                                                     //
//Argument : type             �҂����ԃ^�C�v                                  //
//           waitTime         ���Ԏw��                                        //
//Return   : -                                                                //
////////////////////////////////////////////////////////////////////////////////
//type=0 �b�P�ʎw�� waitTime=100�Ŗ�1�b�҂�                                   //
//type=1 �A�[�����[�^��]�f�B���C                                             //
//type=2 �E�F�[�n���[�^��]�f�B���C                                           //
////////////////////////////////////////////////////////////////////////////////
void    Wait(int type, int waitTime)
{
    int     t;                                                                   //�����ϐ�t���`
    int     i;                                                                   //�����ϐ�i���`

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
//Details  : �A�[�����[�^�쓮                                                 //
//Argument : setPosition               �A�[���̐�Έʒu�w��                   //
//Return   : -                                                                //
////////////////////////////////////////////////////////////////////////////////
void    DriveArmMotor(long int setPosition)
{
    P1.DR.BYTE = 0x00;                                                            //�}�C�R���|�[�g������

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
//Details  : �E�F�[�n���[�^�쓮                                               //
//Argument : rotationDirection          ��]���� [0:CW, 1:CCW]                //
//           pulse                      �p���X��                              //
//Return   : -                                                                //
////////////////////////////////////////////////////////////////////////////////
void    DriveWaferMotor(int rotationDirection, int pulse)
{
    P2.DR.BYTE =0x00;                                                            //�}�C�R���|�[�g������

    /*���v���(CW)*/
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

    /*�����v���(CCW)*/
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
//Details  : AD�R���o�[�^                                                     //
//Argument : -                                                                //
//Return   : temp                       �ϊ���f�[�^                          //
////////////////////////////////////////////////////////////////////////////////

int     ADConvert(void)
{
    int     temp;                                                                //�ϊ��f�[�^�i�[�ϐ�

    AD.ADCSR.BIT.ADF = 0;                                                        //ADF�t���O�N���A
    AD.ADCSR.BIT.SCAN = 0;                                                       //�P�ꃂ�[�h
    AD.ADCSR.BIT.CKS = 1;                                                        //�N���b�N�Z���N�g 134���[�h
    AD.ADCSR.BIT.CH = 0;                                                         //�`�����l�� AN0�P�ꃂ�[�h(P128�Q��)

    AD.ADCSR.BIT.ADST = 1;                                                       //AD�ϊ��J�n

    while (AD.ADCSR.BIT.ADF == 0);                                               //�ϊ��I���܂ő҂�

    temp = AD.ADDRA >> 6;                                                        //�E��6BIT�V�t�g(P127�Q��)�厖�I

    return (temp);
}

////////////////////////////////////////////////////////////////////////////////
//Name     : ControlCylinder                                                  //
//Details  : �V�����_����                                                     //
//Argument : function                   ����ݒ�[0:���~, 1:�㏸]              //
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
//Details  : �ΐc����                                                         //
//Argument : -                                                                //
//Return   : -                                                                //
////////////////////////////////////////////////////////////////////////////////
void    Measurement(void)
{
    int     cnt = 0;                                                             //�J�E���^

    for (cnt = 0; cnt < DEV; cnt++)
    {
        MeasurementData[cnt] = ADConvert();                                      //�f�[�^���f�W�^���l(0 - 1023)�փR���o�[�g
        DriveWaferMotor(0,1);                                                    //�E�F�[�n���[�^��1�p���X��]
    }


    return 0;
}

////////////////////////////////////////////////////////////////////////////////
//Name     : WaitStart                                                        //
//Details  : �����J�n�҂��i�X�C�b�`�����j                                     //
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
//Details  : �m�b�`���o                                                       //
//Argument : -                                                                //
//Return   : -                                                                //
////////////////////////////////////////////////////////////////////////////////
void    FindNotch(void)
{
    int     cnt;                                                                 //�J�E���^
    int     flag = 0;                                                            //���o�I���t���O
    int     max = 0;                                                             //�ꎞ�l�i�[�ϐ�
    int     test;                                                                //�����ϐ�

    /*�m�b�`��������*/
    for (cnt = 0; cnt < (DEV-2); cnt++)
    {
        test = MeasurementData[cnt+2] - MeasurementData[cnt];                    //�ΐc�̕ψʂ���


        /*臒l�����傫�ȕω��ł���΃m�b�`���o�J�n*/
        if (fabs(test) > max)
        {
            max = fabs(test);
            NotchDegree = cnt;                                                   //�m�b�`�̊p�x(�p���X��)��ۑ�
            NotchDetectionFlag = 1;                                              //�m�b�`���o�t���O��ON��

         }

        /*���o�ł��Ȃ������ꍇ*/

        if (cnt == (DEV - 2))
        {
            NotchDetectionFlag = 0;                                              //�m�b�`���o�t���O��OFF��
            break;
        }

    }
    return 0;
}

////////////////////////////////////////////////////////////////////////////////
//Name     : FindError                                                        //
//Details  : �ΐc���o                                                         //
//Argument : -                                                                //
//Return   : -                                                                //
////////////////////////////////////////////////////////////////////////////////
void    FindError(void)
{
    int     cnt;                                                                 //�J�E���^
    int     max;                                                                 //�ő�l�i�[�ϐ�
    int     normalizeData[DEV];                                                  //���K���f�[�^�i�[�z��

    /*�z��𐳋K�����Đ�Βl�����*/
    for (cnt = 0; cnt <DEV; cnt++)
    {
        normalizeData[cnt] = fabs(MeasurementData[cnt] - 482);                   //��Βl�����
    }

    /*�ΐc���o�J�n*/
    if (NotchDegree < (DEV/2))                                                   //�m�b�`��0�`�΂܂ł̂Ƃ�
    {
        max = normalizeData[800];

        for (cnt = DEV/2; cnt < DEV; cnt++)
        {
            /*�ΐc���ő�ƂȂ�p�x��������*/
            if (max < normalizeData[cnt])
            {
                max = normalizeData[cnt];
                ErrorDegree = cnt;                                               //�ΐc�p�x���O���[�o���ϐ��֑��
                ErrorDistance = MeasurementData[cnt] - 482;                      //�ΐc�������O���[�o���ϐ��֑��
            }
        }
    }

    else                                                                         //�m�b�`���΁`2�΂܂ł̂Ƃ�
    {
        max = normalizeData[0];

        for (cnt = 0; cnt < (DEV/2); cnt++)
        {
            /*�ΐc���ő�ƂȂ�p�x��������*/
            if (max < normalizeData[cnt])
            {
                max = normalizeData[cnt];
                ErrorDegree = cnt;                                               //�ΐc�p�x���O���[�o���ϐ��֑��
                ErrorDistance = MeasurementData[cnt] - 482;                      //�ΐc�������O���[�o���ϐ��֑��
            }
        }
    }

    return 0;
}

////////////////////////////////////////////////////////////////////////////////
//Name     : ModifyError                                                      //
//Details  : �ΐc�C��                                                         //
//Argument : -                                                                //
//Return   : -                                                                //
////////////////////////////////////////////////////////////////////////////////
void    ModifyError(void)
{

    long int     distancePulse;                                                  //�C������(�p���X��)
    int     cnt;                                                                 //�J�E���^

    /*�ΐc�������Z�i�Z���T�l���p���X���j*/
    distancePulse = (ErrorDistance * 7.8125) ;      //�C���p���X���Z�o

    /*�ΐc�p�x�C��*/
    DriveWaferMotor(0, ErrorDegree);

    Wait(0,100);

    /*�ΐc�����C��*/
    ControlCylinder(1);                                                          //�V�����_���グ��
    Wait(0,50);
    DriveArmMotor(distancePulse);                                                //�����C��
    Wait(0,50);
    ControlCylinder(0);                                                          //�V�����_��������
    Wait(0,50);
    Wait(0,100);

     /*�m�b�`�̈ʒu�����킹��*/
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

    /*����f�[�^�i�[�z�񏉊���*/
    for (cnt = 0; cnt<DEV; cnt++)
    {
        MeasurementData[cnt] = 0;
    }
    /*�O���[�o���ϐ���������*/
    ErrorDegree = 0;
    ErrorDistance = 0;

    return 0;
}

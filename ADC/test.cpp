#include "interface.h"
#include <iostream>
#include <string>
#include <vector>
#include <string.h>
#include <math.h>

/********************Tester Resource V03 hardware
pin1 CS dio1
pin2 CH0 cbit2 on to dio2, cbit2 off and cbit1 on to awg0
pin3 CH1 cbit2 on to dio3, cbit2 off and cbit1 off to awg0
pin4 GND
pin5 DI dio5
pin6 DO dio6
pin7 CLK dio7
pin8 VCC cbit0 off to dps0, cbit0 on to dio8
*/

/*
DPS output range -3.25~8.4V
Range:
±500mA，resolution 50uA；
±25mA，resolution 2.5uA；
±2.5mA，resolution 250nA；
±250uA，resolution 25nA；
±25uA，resolution 2.5nA；
±5uA，resolution 0.5nA；

PPMU range -2.0~6.5V
-40mA ~ +40mA，分辨率10uA；
-1mA ~ +1mA，分辨率200nA；
-100uA ~ +100uA，分辨率20nA；
-10uA ~ +10uA，分辨率2nA；
-2uA ~ +2uA，分辨率0.4nA；

 */

using namespace std;
unsigned long long ullSite;
unsigned long SITE = 0,SITE_ID = 0,ACTIVE_SITE_NUM = 0;

unsigned int reverse(unsigned int num)
{
	int y = 0;
	for (int i = 0; i < 8; i++)
	{
		y = (y << 1) | (num & 1);
		num >>= 1;
	}
	return y;
}

void POWER_ON(double vcc_volt)
{
	dps.Signal("VCC").SetMode("FVMI").VoltForce(0.0).CurrRange(500e-3).Execute();
	dps.Signal("VCC").Connect();
	sys.DelayUs(8000);
	dps.Signal("VCC").SetMode("FVMI").VoltForce(vcc_volt).CurrClamp(0.5,-0.5).CurrRange(0.5).Execute();
}

void POWER_OFF()
{
	pinlevel.Signal("DIO_ALLPINS").SetMode("DIO").SetVih(0).SetVil(0).SetVoh(0).SetVol(0).VoltClamp(6,-1).Execute();

	ppmu.Signal("DIO_ALLPINS").SetMode("FVMI").VoltForce(0.0).CurrRange(30e-3).Execute();
	sys.DelayUs(3000);
	ppmu.Signal("DIO_ALLPINS").SetMode("FNMV").Execute();

	dps.Signal("VCC").SetMode("FVMI").VoltForce(0.0).CurrRange(500e-3).Execute();
	sys.DelayUs(8000);
	dps.Signal("VCC").DisConnect();
	pinlevel.DisConnect();
	ppmu.DisConnect();
	cbit.Signal("K_ALL").SetOff();
}

USER_CODE void ProjectReuse() {
    cout << "ProjectReuse" << endl;
}

USER_CODE void ProjectInit() {
    cout << "ProjectInit" << endl;
    //cout << "ATE_VERSION:V207" << endl;
}

USER_CODE void ProjectReset() {
    cout << "ProjectReset" << endl;
}

USER_CODE void site_init() {
    cout << "site_init" << endl;

}

USER_CODE void site_reset() {
    cout << "site_reset" << endl;
}

USER_CODE void OS_TEST() {

	vector<ST_MEAS_RESULT> N_PPMU_RESULT;
	vector<ST_MEAS_RESULT> P_PPMU_RESULT;

	POWER_ON(0.0);

	cout << " OSN test start " << endl;

	cbit.Signal("K_OSN").SetOn();
	ppmu.Signal("OSN_PINS").Connect();
	ppmu.Signal("OSN_PINS").SetMode("FVMI").VoltForce(0.0).CurrRange(5.0e-4).VoltClamp(2, -1.5).Execute();
	sys.DelayUs(10000);

	ppmu.Signal("OSN_PINS").SetMode("FIMV").CurrForce(-2.0e-4).CurrRange(5.0e-4).VoltClamp(2,-1.5).Execute();
	sys.DelayUs(1000);
	ppmu.Measure(N_PPMU_RESULT);
	binObj.CheckResultAndBin(0,N_PPMU_RESULT,7);

	ppmu.Signal("OSN_PINS").SetMode("FIMV").CurrForce(0).CurrRange(5.0e-4).VoltClamp(2,-1.5).Execute();
	ppmu.Signal("OSN_PINS").SetMode("FVMI").VoltForce(0.0).CurrRange(500e-6).Execute();
	ppmu.Signal("OSN_PINS").DisConnect();
	cbit.Signal("K0").SetOff();
	sys.DelayUs(5000);

	POWER_ON(0.0);
	sys.DelayUs(1000);
	ppmu.Signal("OSP_PINS").Connect();
	ppmu.Signal("OSP_PINS").SetMode("FIMV").CurrForce(2.0e-4).CurrRange(5.0e-4).VoltClamp(2,-2).Execute();
	sys.DelayUs(1000);
	ppmu.Measure(P_PPMU_RESULT);

	POWER_OFF();
    cbit.Signal("K2").SetOff();
    sys.DelayUs(5000);
    ppmu.Signal("OSP_PINS").SetMode("FIMV").CurrForce(0).CurrRange(5.0e-4).VoltClamp(2,-2).Execute();

	binObj.CheckResultAndBin(7,P_PPMU_RESULT,6);

}

USER_CODE void ADC_Static_AWG() {
	TEST_BEGIN

	pinlevel.Signal("ADC_Digitals").Connect();
	POWER_ON(5.0);
	sys.DelayUs(8000);

	pinlevel.Signal("ADC_Digitals").SetMode("DIO")
			.SetVih(5.0).SetVil(0.0)
			.SetVoh(2.5).SetVol(2.5).SetIoh(0).SetIol(0).SetVt(0)
			.Execute();

	awg.Signal("AWG_CH").Connect();
	awg.Signal("AWG_CH").GenerateTypWave("RAMP", "ADC_RAMP", 409600, 0, 4,0,0);
	awg.LoadWaveFile("ADC_RAMP");
	awg.SetMode(2, 0, 1, 409600,2,0,0);
	awg.SetAmpl(4.98);

	sys.DelayUs(8000);
	dio.Pattern("ADC_CH1_AWG").SetHRamMode(EN_CAP_MODE);

	dio.Pattern("ADC_CH1_AWG").DSIO().SetCaptureMode(DSIOModeSerial)
									 .SetAlignMode(MSB_MODE)
									 .SetSampleSignal("DO")
									 .SetSampleCycleCount(1)
									 .SetSampleBits(8)
									 .SetSampleEdge(2048);
	dio.Pattern("ADC_CH1_AWG").SetTimeOut(20000).Run().WaitEnd();
	vector<UINT> ADCOUT;
	dio.Pattern("ADC_CH1_AWG").DSIO().GetData(ADCOUT);

	awg.Stop();
	POWER_OFF();

	int final_data[2048];

	for(int i =0;i<2048; i++)
	{
		final_data[i] = ADCOUT[i] ;

	}

	int capture_count = 2048;
	int adc_bit = 8;
	int total_output = 1 << adc_bit;

	int trip_point[total_output];
	int code_count[total_output];
	int miss_code=0;
	int first_p = -1;

	for(int i = 0; i < total_output; i++)
	{
		code_count[i] = 0;

		for(int k = 0; k < capture_count; k++)
		{
			if(final_data[k] == i)
			{
				first_p = k;
				break;
			}
		}

		for(int k = 0; k < capture_count; k++)
		{
			if(final_data[k] == i)
			{
				code_count[i]++;
			}

		}
		if(first_p != -1)
		{
			trip_point[i] = first_p + code_count[i];
		}

	}

	double trip_volt[total_output];
	for(int i=0;i< total_output;i++)
	{
		trip_volt[i] = 0 + (trip_point[i])*(5.0/2048);
		if(code_count[i] == 0)
		{

			miss_code = 1;
		}

	}

	double Vzst = trip_volt[0];
	double Vfst = trip_volt[254];

	double LSBdut = (Vfst - Vzst)/(256-2);

	double offset_error = (Vzst - LSBdut/2)/LSBdut;
	double gain_error = (Vfst -Vzst + 2*LSBdut-5.0)/LSBdut;

	double dnl[total_output];
	double inl[total_output];
	dnl[0] = 0;
	inl[0] = 0;
	double max_dnl = 0;
	double max_inl = 0;
	for(int i=1; i< 255; i++)
	{
		dnl[i] = (trip_volt[i] - trip_volt[i-1]) - LSBdut;
		inl[i] = inl[i-1] + (dnl[i] + dnl[i-1])/2;
		if(abs(dnl[i]) < abs(dnl[i-1]))
		{
			max_dnl = dnl[i-1];
		}
		else
			max_dnl = dnl[i];
		if(abs(inl[i]) < abs(inl[i-1]))
		{
			max_inl = inl[i-1];
		}
		else
			max_inl = inl[i];
	}

	SITE_DOUBLE result;

	result[0] = offset_error;

	binObj.CheckResultAndBin(0, result);

	result[0] = gain_error;

	binObj.CheckResultAndBin(1, result);

	result[0] = miss_code;

	binObj.CheckResultAndBin(2, result);

	result[0] = max_dnl;

	binObj.CheckResultAndBin(3, result);

	result[0] = max_inl;

	binObj.CheckResultAndBin(4, result);

	POWER_OFF();


	POWER_OFF();
	TEST_ERROR
	binObj.HandlerException(0);
	TEST_END
}

USER_CODE void ADC_Dynamic_DSIO() {
	TEST_BEGIN

	pinlevel.Signal("ADC_Digitals").Connect();
	POWER_ON(5.0);
	sys.DelayUs(8000);

	pinlevel.Signal("ADC_Digitals").SetMode("DIO")
			.SetVih(5.0).SetVil(0.0)
			.SetVoh(2.5).SetVol(2.5).SetIoh(0).SetIol(0).SetVt(0)
			.Execute();

	awg.Signal("AWG_CH").Connect();
	awg.Signal("AWG_CH").GenerateTypWave("SIN", "ADC_SIN", 102400, 2,2, 0);
	awg.LoadWaveFile("ADC_SIN");
	awg.SetMode(0x0, 0x0, 1, 102400, 1, 0, 0);
	awg.SetAmpl(5);
	awg.Send();

	sys.DelayUs(8000);
	dio.Pattern("ADC_CH1_DYNAMIC_DSIO").SetHRamMode(EN_CAP_MODE);
	dio.Pattern("ADC_CH1_DYNAMIC_DSIO").DSIO().SetCaptureMode(DSIOModeSerial)
									 .SetAlignMode(MSB_MODE)
									 .SetSampleSignal("DO")
									 .SetSampleCycleCount(1)
									 .SetSampleBits(8)
									 .SetSampleEdge(2048);

	dio.Pattern("ADC_CH1_DYNAMIC_DSIO").SetTimeOut(20000).Test();
	vector<UINT> vctBuff;
	dio.Pattern("ADC_CH1_DYNAMIC_DSIO").DSIO().GetData(vctBuff);

	awg.Stop();
	POWER_OFF();
	double dWave[2048];
	for(int i=0;i<2048;i++)
	{
		dWave[i] = double(vctBuff[i]);
	}
	CWaveCalc ADC_DYN;
	CWaveCalc D;

	ADC_DYN.Create(dWave, 2048);
	D = ADC_DYN.Spectrum();

	double SNR = D.CalcSNR(3, -1);
	double THD = D.CalcTHD(3, -1);
	double SIAND = D.CalcSINAD(3);
	double ENOB = (SIAND -1.76)/6.02;

	SITE_DOUBLE Compare_Value;
	Compare_Value[0] = SNR;
	binObj.CheckResultAndBin(0, Compare_Value);

	Compare_Value[0] = SIAND;
	binObj.CheckResultAndBin(1, Compare_Value);

	Compare_Value[0] = THD;
	binObj.CheckResultAndBin(2, Compare_Value);

	Compare_Value[0] = ENOB;
	binObj.CheckResultAndBin(3, Compare_Value);

	TEST_ERROR
	binObj.HandlerException(0);
	TEST_END
}

void EndOfWafer() {
	cout << "EndOfWafer" << endl;
}

void EndOfLot() {
	cout << "EndOfLot" << endl;
}

inline void Enable_EOW_Func() {
	proberObj.EnableWaferEndCheck(EndOfWafer);
}

inline void Enable_EOL_Func() {
	proberObj.EnableLotEndCheck(EndOfLot);
}

USER_CODE void SetPassBin() {
	TEST_BEGIN

	// TODO Edit your code here
	binObj.SetBin(1);

	TEST_ERROR
	binObj.HandlerException(0);
	TEST_END
}

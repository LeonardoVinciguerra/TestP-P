//---------------------------------------------------------------------------
// Name:        q_carobj.h
// Author:      
// Created:     
// Description: Gestione caricatori.
//---------------------------------------------------------------------------

#ifndef __Q_CAROBJ_H
#define __Q_CAROBJ_H

#include "q_cost.h"
#include "q_feederfile.h"
#include "c_window.h"

//THFEEDER
#define CARTYPE_TAPE		0
#define CARTYPE_AIR			1
#define CARTYPE_THFEEDER	2
#define CARTYPE_DOME		3

//THFEEDER
#define FEEDER_STEPS_NORMAL 	-80
#define FEEDER_STEPS_MEDIUM 	-160
#define FEEDER_STEPS_LONG 		-240
#define FEEDER_STEPS_DOUBLE 	-40



class FeederClass
{
private:
	FeederFile* file;
	CarDat car;
	unsigned char StartNComp; // numero di conponenti da prelevare ad ogni avanzamento
	int code;

public:
	virtual int Avanza(int C_att=-99,int test=0);
	int WaitReady(void);
	int CheckReady(void); //THFEEDER
	void DecNComp(void);
	void IncNComp(void);
	void SetNComp(int val);
	int GetQuantity(void);

	void SetQuantity(unsigned int quant);
	
	void Set_StartNComp(void);
	int  Get_StartNComp(void);

	int GetTrayPosition(float& x,float &y);

	void UpdateStatus(void);
	int TeachPosition();
	int GetCode(void);
	virtual void SetCode(int _code);
	virtual void SetRec(int _rec);
	virtual void ReloadData(void);
	bool GoPos( int nozzle );
	CarDat GetData(void);
	CarDat& GetDataRef(void);
	const CarDat& GetDataConstRef(void);
	virtual void SetData(struct CarDat caric);
	int Get_AssociatedPack( struct SPackageData& pack );
	
	#ifdef __DOME_FEEDER
	int DomesForcedUp(void);
	int DomesForcedDown(void);
	#endif
	
	FeederClass(FeederFile *_file,int _code);
	FeederClass(FeederFile *_file);
	FeederClass(const CarDat& rec_data);
};

void make_car(void);

#endif

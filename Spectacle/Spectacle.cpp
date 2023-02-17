//-----------------------------------------------------------------------------
// Entaro ChucK Developer!
// This is a Chugin boilerplate, generated by chuginate!
//-----------------------------------------------------------------------------

/*
 *  Copyright (C) 2008 John Gibson
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License, version 2, as
 *  published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 */

// this should align with the correct versions of these ChucK files
#include "chuck_dl.h"
#include "chuck_def.h"
#include "util_math.h"
#include "ulib_math.h"

#include "Spectacle-dsp.h"

// general includes
#include <stdio.h>
#include <limits.h>

// TODO: due to how chuck random #defines are done, this may need to be __WINDOWS_DS__
#ifdef WIN32
#define random rand
#endif

// declaration of chugin constructor
CK_DLL_CTOR(spectacle_ctor);
// declaration of chugin desctructor
CK_DLL_DTOR(spectacle_dtor);

CK_DLL_MFUN(spectacle_setFFTlen);
CK_DLL_MFUN(spectacle_getFFTlen);

CK_DLL_MFUN(spectacle_setOverlap);
CK_DLL_MFUN(spectacle_getOverlap);

CK_DLL_MFUN(spectacle_setMaxDelay);
CK_DLL_MFUN(spectacle_getMaxDelay);

CK_DLL_MFUN(spectacle_setMinDelay);
CK_DLL_MFUN(spectacle_getMinDelay);

CK_DLL_MFUN(spectacle_clear);

CK_DLL_MFUN(spectacle_setHold);
CK_DLL_MFUN(spectacle_getHold);

CK_DLL_MFUN(spectacle_setPostEQ);
CK_DLL_MFUN(spectacle_getPostEQ);

CK_DLL_MFUN(spectacle_setMinFreq);
CK_DLL_MFUN(spectacle_getMinFreq);

CK_DLL_MFUN(spectacle_setMaxFreq);
CK_DLL_MFUN(spectacle_getMaxFreq);

CK_DLL_MFUN(spectacle_setMix);
CK_DLL_MFUN(spectacle_getMix);

CK_DLL_MFUN(spectacle_setFreqRange);

CK_DLL_MFUN(spectacle_setTable);

CK_DLL_MFUN(spectacle_setTableLen);
CK_DLL_MFUN(spectacle_getTableLen);

CK_DLL_MFUN(spectacle_setDelay);
CK_DLL_MFUN(spectacle_setEQ);
CK_DLL_MFUN(spectacle_setFB);

// for Chugins extending UGen, this is mono synthesis function for 1 sample
CK_DLL_TICKF(spectacle_tick);

// this is a special offset reserved for Chugin internal data
t_CKINT spectacle_data_offset = 0;

#define CONFORM_INPUT
#define CLIP(a, lo, hi) ( (a)>(lo)?( (a)<(hi)?(a):(hi) ):(lo) )
  
#define kDefaultFFTLen           1024
#define kDefaultWindowLen        2048
#define kDefaultOverlap          2
#define kDefaultMaxDelTime       6.0f
#define kMinMaxDelTime           0.1f
#define kMaxMaxDelTime           20.0f
#define kMaxTableLen             512
#define kDefaultTableSize        64

// class definition for internal Chugin data
// (note: this isn't strictly necessary, but serves as example
// of one recommended approach)
class Spectacle
{
public:
  // constructor
  Spectacle( t_CKFLOAT fs)
  {
    fftlen = kDefaultFFTLen;
    windowlen = kDefaultWindowLen;
    overlap = kDefaultOverlap;
    maxdeltime = kDefaultMaxDelTime;
    maxdeltime = CLIP(maxdeltime, kMinMaxDelTime, kMaxMaxDelTime);
	mindeltime = 0.0;
    srate = fs;
    mix = 1.0;
    spectdelay = NULL;
    float *del = dttable;
    float *eq = eqtable;
    float *feed = fbtable;
    for (int i = 0; i < kMaxTableLen; i++)
      {
	*del++ = rand2f(mindeltime, maxdeltime);
	*eq++ = 0.0;
	*feed++ = 0.0;
      }
    dttablen = eqtablen = fbtablen = kDefaultTableSize;
    minfreq = maxfreq = 0.0;
    posteq = hold = false;
    
    spectdelay = new Spectacle_dsp();
    spectdelay->init(fftlen, windowlen, overlap, srate, maxdeltime);
	spectdelay->set_deltable(dttable,dttablen);
	spectdelay->set_eqtable(eqtable,eqtablen);
	spectdelay->set_feedtable(fbtable,fbtablen);
  }

  ~Spectacle()
  {
    delete spectdelay;
  }
 
  // for Chugins extending UGen
  void tick( SAMPLE* in, SAMPLE* out, int nframes)
  {
    memset (out, 0, sizeof(SAMPLE)*nframes);
	for (int i=0; i<nframes; i+=2)
	  {
		spectdelay->run(in+i, out+i, 1);
		spectdelay->run(in+i, out+i+1, 1);
		out[i] = out[i] * mix + in[i] * (1-mix);
		out[i+1] = out[i+1] * mix + in[i+1] * (1-mix);
	  }
    // process a block of samples
    //spectdelay->run(in, out, nframes);
  }

  void clear()
  {
	//printf ( "Spectacle: cleared.\n" );
    spectdelay->clear();
  }
  
  float setMaxDelay ( t_CKDUR p)
  {
    float dt = (p / srate);
    dt = CLIP(dt, kMinMaxDelTime, kMaxMaxDelTime);
	if (dt > maxdeltime)
	  {
		// pin existing delay times to new maximum
		for (int i = 0; i < dttablen; i++)
		  {
			if (dttable[i] > dt)
			  dttable[i] = dt;
		  }
		spectdelay->set_maxdeltime(dt);
	  }
    maxdeltime = dt;
	//printf("dt: %f, maxdeltime: %f\n",dt,maxdeltime);
    return p;
  }

  float getMaxDelay ()
  {
    return (maxdeltime * srate);
  }

  float setMinDelay ( t_CKDUR p)
  {
    float dt = (p / srate);
    mindeltime = CLIP(dt, 0.0, maxdeltime);
    return p;
  }

  float getMinDelay ()
  {
    return (mindeltime * srate);
  }

  // set parameter example
  int setHold( t_CKINT p )
  {
	if (p > 1 || p < 0)
	  {
		p = 1;
		printf ("Spectacle: hold must be 0 or 1\n");
	  }
	hold = p;
	spectdelay->set_hold(p);
    return p;
  }
  
  // get parameter example
  int getHold() { return hold; }

  // set parameter example
  int setPostEQ( t_CKINT p )
  {
	if (p > 1 || p < 0)
	  {
		p = 1;
		printf ("Spectacle: posteq must be 0 or 1\n");
	  }
	posteq = p;
	spectdelay->set_posteq(p);
    return p;
  }
  
  // get parameter example
  int getPostEQ() { return posteq; }

  // set parameter example
  int setFFTlen( t_CKINT p )
  {
	clear();
	int newfft = 1;
	// ensure it's a power of 2
	while (newfft < p) newfft = newfft << 1;
	fftlen = newfft;
	windowlen = newfft * 2;
    spectdelay->init(fftlen, windowlen, overlap, srate, maxdeltime);
    return fftlen;
  }

  int getFFTlen() { return fftlen; };

  // set parameter example
  int setOverlap( t_CKINT p )
  {
	clear();
	overlap = p;
    spectdelay->init(fftlen, windowlen, overlap, srate, maxdeltime);
    return overlap;
  }

  int getOverlap() { return overlap; };

  float setMinFreq ( t_CKFLOAT p )
  {
	float min = CLIP(p, 0.0, maxfreq);
	if (min != minfreq)
	  {
		minfreq = min;
		spectdelay->set_delay_freqrange(minfreq, maxfreq);
		spectdelay->set_freqrange(minfreq, maxfreq);
	  }
      
    return min;
  }

  float getMinFreq () { return minfreq; }

  float setMaxFreq ( t_CKFLOAT p )
  {
	const float nyquist = srate / 2;
	if (p == 0) p = nyquist;
	float max = CLIP(p, minfreq, nyquist);
	if (max != maxfreq)
	  {
		maxfreq = max;
		spectdelay->set_delay_freqrange(minfreq, maxfreq);
		spectdelay->set_freqrange(minfreq, maxfreq);
	  }
    
    return max;
  }

  float getMaxFreq () { return maxfreq; }

  float setMix ( t_CKFLOAT p )
  {
    mix = CLIP(p,0,1);
    return mix;
  }

  float getMix () { return mix; }

  void setFreqRange ( t_CKFLOAT q, t_CKFLOAT p )
  {
	const float nyquist = srate / 2;
	float min = CLIP(p, 0.0, nyquist);
	if (q == 0) q = nyquist;
	float max = CLIP(q, min, nyquist);
	if (max != maxfreq || min != minfreq)
	  {
		maxfreq = max;
		minfreq = min;
		spectdelay->set_delay_freqrange(minfreq, maxfreq);
		spectdelay->set_freqrange(minfreq, maxfreq);
	  }
  }
  
  // set parameter example
  int setTableLen( t_CKINT p )
  {
	if (p > kMaxTableLen)
	  {
		printf("Spectacle: tableSize cannot exceed %d.\n",kMaxTableLen);
	  }
	p = CLIP(p,1,kMaxTableLen);
	dttablen = p;
	fbtablen = p;
	eqtablen = p;
    return p;
  }

  int getTableLen() { return dttablen; };

  float setDelay (t_CKDUR p)
  {
	float x = (p/srate);
	for (int i=0; i<dttablen; i++)
	  {
		dttable[i] = x;
	  }
	spectdelay->set_deltable(dttable,dttablen);
	return p;
  }

  float setEQ (t_CKFLOAT x)
  {
	for (int i=0; i<eqtablen; i++)
	  {
		eqtable[i] = x;
	  }
	spectdelay->set_eqtable(eqtable,eqtablen);
	return x;
  }

  float setFB (t_CKFLOAT x)
  {
	for (int i=0; i<fbtablen; i++)
	  {
		fbtable[i] = x;
	  }
	spectdelay->set_feedtable(fbtable,fbtablen);
	return x;
  }

    int setTable ( const char* p, const char* q )
  {
	const char p0 = p[0];
	const char q0 = q[0];
	int table, type;

	switch (p0)
	  {
	  case 'a':
		type = 0;
		break;
	  case 'd':
		type = 1;
		break;
	  case 'e':
		type = 2;
		break;
	  case 'f':
		type = 3;
		break;
	  default:
		printf ("Spectacle: error: first argument \"%s\" not valid.\n",p);
		return 0;
	  }

	switch (q0)
	  {
	  case 'a':
		type = 0;
		break;
	  case 'd':
		type = 1;
		break;
	  case 'r':
		type = 2;
		break;
	  default:
		printf ("Spectacle: error: second argument \"%s\" not valid.\n",q);
		return 0;
	  }

	if (table == 0 || table == 1) setDelayTable(type);
	if (table == 0 || table == 2) setEQTable(type);
	if (table == 0 || table == 3) setFBTable(type);
	spectdelay->set_deltable(dttable,dttablen);
	spectdelay->set_eqtable(eqtable,eqtablen);
	spectdelay->set_feedtable(fbtable,fbtablen);

	return 1;
  }
		
  
private:
  int fftlen, windowlen, overlap;
  float srate, maxdeltime, mindeltime;
  Spectacle_dsp *spectdelay;
  float eqtable[kMaxTableLen];
  float dttable[kMaxTableLen];
  float fbtable[kMaxTableLen];
  int eqtablen, dttablen, fbtablen;
  float minfreq, maxfreq;
  bool hold, posteq;
  float mix;

  void setDelayTable (int type)
  {
	for (int i=0; i<dttablen; i++)
	  {
		switch (type)
		  {
		  case 0:
			dttable[i] = lerp((float)i, 0, (float)dttablen, mindeltime, maxdeltime);
			break;
		  case 1:
			dttable[i] = lerp((float)i, (float)dttablen, 0, mindeltime, maxdeltime);
			break;
		  default:
			dttable[i] = rand2f(mindeltime, maxdeltime);
		  }
	  }
  }

  void setEQTable (int type)
  {
	for (int i=0; i<eqtablen; i++)
	  {
		switch (type)
		  {
		  case 0:
			eqtable[i] = lerp((float)i, 0, (float)dttablen, -24.f,24.f);
			break;
		  case 1:
			eqtable[i] = lerp((float)i, (float)dttablen, 0, -24.f,24.f);
			break;
		  default:
			eqtable[i] = rand2f(-24.f,24.f);
		  }
	  }
  }

  void setFBTable (int type)
  {
	for (int i=0; i<fbtablen; i++)
	  {
		switch (type)
		  {
		  case 0:
			fbtable[i] = lerp((float)i, 0, (float)fbtablen, -1.0 , 1.0);
			break;
		  case 1:
			fbtable[i] = lerp((float)i, (float)fbtablen, 0, -1.0, 1.0);
			break;
		  default:
			fbtable[i] = rand2f( -0.1, 0.1 );
		  }
	  }
  }

  float rand2f (float min, float max)
  {
    return min + (max-min)*(::random()/(t_CKFLOAT)CK_RANDOM_MAX);
  }

  float lerp (float val, float inlow, float inhigh, float outlow, float outhigh)
  {
	return outlow + (val-inlow) * (outhigh-outlow) / (inhigh-inlow);
	}
};


// query function: chuck calls this when loading the Chugin
// NOTE: developer will need to modify this function to
// add additional functions to this Chugin
CK_DLL_QUERY( Spectacle )
{
  // hmm, don't change this...
  QUERY->setname(QUERY, "Spectacle");
  
  // begin the class definition
  // can change the second argument to extend a different ChucK class
  QUERY->begin_class(QUERY, "Spectacle", "UGen");
  
  // register the constructor (probably no need to change)
  QUERY->add_ctor(QUERY, spectacle_ctor);
  // register the destructor (probably no need to change)
  QUERY->add_dtor(QUERY, spectacle_dtor);
  
  // for UGen's only: add tick function
  QUERY->add_ugen_funcf(QUERY, spectacle_tick, NULL, 2, 2);
  
  // NOTE: if this is to be a UGen with more than 1 channel, 
  // e.g., a multichannel UGen -- will need to use add_ugen_funcf()
  // and declare a tickf function using CK_DLL_TICKF
  
  // example of adding setter method
  QUERY->add_mfun(QUERY, spectacle_clear, "void", "clear");

  // example of adding setter method
  QUERY->add_mfun(QUERY, spectacle_setHold, "int", "hold");
  // example of adding argument to the above method
  QUERY->add_arg(QUERY, "int", "arg");

  // example of adding setter method
  QUERY->add_mfun(QUERY, spectacle_getHold, "int", "hold");

  // example of adding setter method
  QUERY->add_mfun(QUERY, spectacle_setPostEQ, "int", "posteq");
  // example of adding argument to the above method
  QUERY->add_arg(QUERY, "int", "arg");

  // example of adding setter method
  QUERY->add_mfun(QUERY, spectacle_getPostEQ, "int", "posteq");

  // example of adding setter method
  QUERY->add_mfun(QUERY, spectacle_setFFTlen, "int", "fftlen");
  // example of adding argument to the above method
  QUERY->add_arg(QUERY, "int", "arg");

  // example of adding setter method
  QUERY->add_mfun(QUERY, spectacle_getFFTlen, "int", "fftlen");

  // example of adding setter method
  QUERY->add_mfun(QUERY, spectacle_setOverlap, "int", "overlap");
  // example of adding argument to the above method
  QUERY->add_arg(QUERY, "int", "arg");

  // example of adding setter method
  QUERY->add_mfun(QUERY, spectacle_getOverlap, "int", "overlap");

  // example of adding setter method
  QUERY->add_mfun(QUERY, spectacle_setMaxDelay, "dur", "delayMax");
  // example of adding argument to the above method
  QUERY->add_arg(QUERY, "dur", "delay");

  // example of adding setter method
  QUERY->add_mfun(QUERY, spectacle_getMaxDelay, "dur", "delayMax");

  // example of adding setter method
  QUERY->add_mfun(QUERY, spectacle_setMinDelay, "dur", "delayMin");
  // example of adding argument to the above method
  QUERY->add_arg(QUERY, "dur", "delay");

  // example of adding setter method
  QUERY->add_mfun(QUERY, spectacle_getMinDelay, "dur", "delayMin");

  // example of adding setter method
  QUERY->add_mfun(QUERY, spectacle_setMinFreq, "float", "freqMin");
  // example of adding argument to the above method
  QUERY->add_arg(QUERY, "float", "arg");

  // example of adding getter method
  QUERY->add_mfun(QUERY, spectacle_getMinFreq, "float", "freqMin");

  // example of adding setter method
  QUERY->add_mfun(QUERY, spectacle_setMaxFreq, "float", "freqMax");
  // example of adding argument to the above method
  QUERY->add_arg(QUERY, "float", "arg");

  // example of adding getter method
  QUERY->add_mfun(QUERY, spectacle_getMaxFreq, "float", "freqMax");

  // example of adding setter method
  QUERY->add_mfun(QUERY, spectacle_setMix, "float", "mix");
  // example of adding argument to the above method
  QUERY->add_arg(QUERY, "float", "mix");

  // example of adding getter method
  QUERY->add_mfun(QUERY, spectacle_getMix, "float", "mix");

  // example of adding setter method
  QUERY->add_mfun(QUERY, spectacle_setFreqRange, "void", "range");
  // example of adding argument to the above method
  QUERY->add_arg(QUERY, "float", "arg1");
  QUERY->add_arg(QUERY, "float", "arg2");

  // example of adding setter method
  QUERY->add_mfun(QUERY, spectacle_setTableLen, "int", "bands");
  // example of adding argument to the above method
  QUERY->add_arg(QUERY, "int", "arg");

  // example of adding setter method
  QUERY->add_mfun(QUERY, spectacle_getTableLen, "int", "bands");

  // example of adding setter method
  QUERY->add_mfun(QUERY, spectacle_setTable, "int", "table");
  // example of adding argument to the above method
  QUERY->add_arg(QUERY, "string", "table");
  QUERY->add_arg(QUERY, "string", "type");

  // example of adding setter method
  QUERY->add_mfun(QUERY, spectacle_setDelay, "dur", "delay");
  // example of adding argument to the above method
  QUERY->add_arg(QUERY, "dur", "delay");

  // example of adding setter method
  QUERY->add_mfun(QUERY, spectacle_setEQ, "float", "eq");
  // example of adding argument to the above method
  QUERY->add_arg(QUERY, "float", "eq");

  // example of adding setter method
  QUERY->add_mfun(QUERY, spectacle_setFB, "float", "feedback");
  // example of adding argument to the above method
  QUERY->add_arg(QUERY, "float", "feedback");
  
  // this reserves a variable in the ChucK internal class to store 
  // referene to the c++ class we defined above
  spectacle_data_offset = QUERY->add_mvar(QUERY, "int", "@s_data", false);
  
  // end the class definition
  // IMPORTANT: this MUST be called!
  QUERY->end_class(QUERY);
  
  // wasn't that a breeze?
  return TRUE;
}


// implementation for the constructor
CK_DLL_CTOR(spectacle_ctor)
{
  // get the offset where we'll store our internal c++ class pointer
  OBJ_MEMBER_INT(SELF, spectacle_data_offset) = 0;
  
  // instantiate our internal c++ class representation
  Spectacle * bcdata = new Spectacle(API->vm->get_srate(API, SHRED));
  
  // store the pointer in the ChucK object member
  OBJ_MEMBER_INT(SELF, spectacle_data_offset) = (t_CKINT) bcdata;
}


// implementation for the destructor
CK_DLL_DTOR(spectacle_dtor)
{
  // get our c++ class pointer
  Spectacle * bcdata = (Spectacle *) OBJ_MEMBER_INT(SELF, spectacle_data_offset);
  // check it
  if( bcdata )
    {
      // clean up
      delete bcdata;
      OBJ_MEMBER_INT(SELF, spectacle_data_offset) = 0;
      bcdata = NULL;
    }
}


// implementation for tick function
CK_DLL_TICKF(spectacle_tick)
{
  // get our c++ class pointer
  Spectacle * c = (Spectacle *) OBJ_MEMBER_INT(SELF, spectacle_data_offset);
  
  // invoke our tick function; store in the magical out variable
  if(c) c->tick(in,out, nframes);

  // yes
  return TRUE;
}

// example implementation for setter
CK_DLL_MFUN(spectacle_setMaxDelay)
{
  // get our c++ class pointer
  Spectacle * bcdata = (Spectacle *) OBJ_MEMBER_INT(SELF, spectacle_data_offset);
  // set the return value
  RETURN->v_dur = bcdata->setMaxDelay(GET_NEXT_DUR(ARGS));
}

// example implementation for setter
CK_DLL_MFUN(spectacle_getMaxDelay)
{
  // get our c++ class pointer
  Spectacle * bcdata = (Spectacle *) OBJ_MEMBER_INT(SELF, spectacle_data_offset);
  // set the return value
  RETURN->v_dur = bcdata->getMaxDelay();
}

// example implementation for setter
CK_DLL_MFUN(spectacle_setMinDelay)
{
  // get our c++ class pointer
  Spectacle * bcdata = (Spectacle *) OBJ_MEMBER_INT(SELF, spectacle_data_offset);
  // set the return value
  RETURN->v_dur = bcdata->setMinDelay(GET_NEXT_DUR(ARGS));
}

// example implementation for setter
CK_DLL_MFUN(spectacle_getMinDelay)
{
  // get our c++ class pointer
  Spectacle * bcdata = (Spectacle *) OBJ_MEMBER_INT(SELF, spectacle_data_offset);
  // set the return value
  RETURN->v_dur = bcdata->getMinDelay();
}

// example implementation for setter
CK_DLL_MFUN(spectacle_clear)
{
  // get our c++ class pointer
  Spectacle * bcdata = (Spectacle *) OBJ_MEMBER_INT(SELF, spectacle_data_offset);
  // set the return value
  bcdata->clear();
}

// example implementation for setter
CK_DLL_MFUN(spectacle_setHold)
{
  // get our c++ class pointer
  Spectacle * bcdata = (Spectacle *) OBJ_MEMBER_INT(SELF, spectacle_data_offset);
  // set the return value
  RETURN->v_int = bcdata->setHold(GET_NEXT_INT(ARGS));
}

// example implementation for setter
CK_DLL_MFUN(spectacle_getHold)
{
  // get our c++ class pointer
  Spectacle * bcdata = (Spectacle *) OBJ_MEMBER_INT(SELF, spectacle_data_offset);
  // set the return value
  RETURN->v_int = bcdata->getHold();
}

// example implementation for setter
CK_DLL_MFUN(spectacle_setPostEQ)
{
  // get our c++ class pointer
  Spectacle * bcdata = (Spectacle *) OBJ_MEMBER_INT(SELF, spectacle_data_offset);
  // set the return value
  RETURN->v_int = bcdata->setPostEQ(GET_NEXT_INT(ARGS));
}

// example implementation for setter
CK_DLL_MFUN(spectacle_getPostEQ)
{
  // get our c++ class pointer
  Spectacle * bcdata = (Spectacle *) OBJ_MEMBER_INT(SELF, spectacle_data_offset);
  // set the return value
  RETURN->v_int = bcdata->getPostEQ();
}

// example implementation for setter
CK_DLL_MFUN(spectacle_setFFTlen)
{
  // get our c++ class pointer
  Spectacle * bcdata = (Spectacle *) OBJ_MEMBER_INT(SELF, spectacle_data_offset);
  // set the return value
  RETURN->v_int = bcdata->setFFTlen(GET_NEXT_INT(ARGS));
}

// example implementation for setter
CK_DLL_MFUN(spectacle_getFFTlen)
{
  // get our c++ class pointer
  Spectacle * bcdata = (Spectacle *) OBJ_MEMBER_INT(SELF, spectacle_data_offset);
  // set the return value
  RETURN->v_int = bcdata->getFFTlen();
}

// example implementation for setter
CK_DLL_MFUN(spectacle_setOverlap)
{
  // get our c++ class pointer
  Spectacle * bcdata = (Spectacle *) OBJ_MEMBER_INT(SELF, spectacle_data_offset);
  // set the return value
  RETURN->v_int = bcdata->setOverlap(GET_NEXT_INT(ARGS));
}

// example implementation for setter
CK_DLL_MFUN(spectacle_getOverlap)
{
  // get our c++ class pointer
  Spectacle * bcdata = (Spectacle *) OBJ_MEMBER_INT(SELF, spectacle_data_offset);
  // set the return value
  RETURN->v_int = bcdata->getOverlap();
}

// example implementation for setter
CK_DLL_MFUN(spectacle_setMinFreq)
{
  // get our c++ class pointer
  Spectacle * bcdata = (Spectacle *) OBJ_MEMBER_INT(SELF, spectacle_data_offset);
  // set the return value
  RETURN->v_float = bcdata->setMinFreq(GET_NEXT_FLOAT(ARGS));
}


// example implementation for getter
CK_DLL_MFUN(spectacle_getMinFreq)
{
  // get our c++ class pointer
  Spectacle * bcdata = (Spectacle *) OBJ_MEMBER_INT(SELF, spectacle_data_offset);
  // set the return value
  RETURN->v_float = bcdata->getMinFreq();
}

// example implementation for setter
CK_DLL_MFUN(spectacle_setMaxFreq)
{
  // get our c++ class pointer
  Spectacle * bcdata = (Spectacle *) OBJ_MEMBER_INT(SELF, spectacle_data_offset);
  // set the return value
  RETURN->v_float = bcdata->setMaxFreq(GET_NEXT_FLOAT(ARGS));
}


// example implementation for getter
CK_DLL_MFUN(spectacle_getMaxFreq)
{
  // get our c++ class pointer
  Spectacle * bcdata = (Spectacle *) OBJ_MEMBER_INT(SELF, spectacle_data_offset);
  // set the return value
  RETURN->v_float = bcdata->getMaxFreq();
}

// example implementation for setter
CK_DLL_MFUN(spectacle_setMix)
{
  // get our c++ class pointer
  Spectacle * bcdata = (Spectacle *) OBJ_MEMBER_INT(SELF, spectacle_data_offset);
  // set the return value
  RETURN->v_float = bcdata->setMix(GET_NEXT_FLOAT(ARGS));
}


// example implementation for getter
CK_DLL_MFUN(spectacle_getMix)
{
  // get our c++ class pointer
  Spectacle * bcdata = (Spectacle *) OBJ_MEMBER_INT(SELF, spectacle_data_offset);
  // set the return value
  RETURN->v_float = bcdata->getMix();
}

// example implementation for setter
CK_DLL_MFUN(spectacle_setFreqRange)
{
  // get our c++ class pointer
  Spectacle * bcdata = (Spectacle *) OBJ_MEMBER_INT(SELF, spectacle_data_offset);
  // set the return value
  t_CKFLOAT rangeMin = GET_NEXT_FLOAT(ARGS);
  t_CKFLOAT rangeMax = GET_NEXT_FLOAT(ARGS);
  bcdata->setFreqRange(rangeMin,rangeMax);
}

// example implementation for setter
CK_DLL_MFUN(spectacle_setTable)
{
  // get our c++ class pointer
  Spectacle * bcdata = (Spectacle *) OBJ_MEMBER_INT(SELF, spectacle_data_offset);
  // set the return value
  const char* q = GET_NEXT_STRING(ARGS)->c_str();
  const char* p = GET_NEXT_STRING(ARGS)->c_str();
  RETURN->v_int = bcdata->setTable(q,p);
}

// example implementation for setter
CK_DLL_MFUN(spectacle_setTableLen)
{
  // get our c++ class pointer
  Spectacle * bcdata = (Spectacle *) OBJ_MEMBER_INT(SELF, spectacle_data_offset);
  // set the return value
  RETURN->v_int = bcdata->setTableLen(GET_NEXT_INT(ARGS));
}

// example implementation for setter
CK_DLL_MFUN(spectacle_getTableLen)
{
  // get our c++ class pointer
  Spectacle * bcdata = (Spectacle *) OBJ_MEMBER_INT(SELF, spectacle_data_offset);
  // set the return value
  RETURN->v_int = bcdata->getTableLen();
}

// example implementation for setter
CK_DLL_MFUN(spectacle_setDelay)
{
  // get our c++ class pointer
  Spectacle * bcdata = (Spectacle *) OBJ_MEMBER_INT(SELF, spectacle_data_offset);
  // set the return value
  RETURN->v_dur = bcdata->setDelay(GET_NEXT_DUR(ARGS));
}

// example implementation for setter
CK_DLL_MFUN(spectacle_setEQ)
{
  // get our c++ class pointer
  Spectacle * bcdata = (Spectacle *) OBJ_MEMBER_INT(SELF, spectacle_data_offset);
  // set the return value
  RETURN->v_float = bcdata->setEQ(GET_NEXT_FLOAT(ARGS));
}

// example implementation for setter
CK_DLL_MFUN(spectacle_setFB)
{
  // get our c++ class pointer
  Spectacle * bcdata = (Spectacle *) OBJ_MEMBER_INT(SELF, spectacle_data_offset);
  // set the return value
  RETURN->v_float = bcdata->setFB(GET_NEXT_FLOAT(ARGS));
}

/*
 * A fixed alphabet encoder for symbols 0 to 5 inclusive.
 * We have no escape symbol or resorting of data. It simply
 * accumulates stats and encodes in proportion.
 *
 * The intention is to use this per column for encoding
 * bases that differ to the consensus.
 */

/*
 * Fast mode uses 8-bit counters. It's generally 40% faster or so, but less
 * accurate and so larger file size (maybe only 1% though unless deep data).
 */

// Small enough to not overflow uint16_t even after +STEP
#ifdef WSIZ
#  undef WSIZ
#endif

#define M4(a) ((a)[0]>(a)[1]?((a)[0]>(a)[2]?((a)[0]>(a)[3]?(a)[0]:(a)[3]):((a)[2]>(a)[3]?(a)[2]:(a)[3])):((a)[1]>(a)[2]?((a)[1]>(a)[3]?(a)[1]:(a)[3]):((a)[2]>(a)[3]?(a)[2]:(a)[3])))

template <typename st_t>
struct BASE_MODEL {
    enum { STEP = sizeof(st_t) == 1 ? 1 : 8 };
    enum { WSIZ = (1<<8*sizeof(st_t))-2*STEP };
    
    BASE_MODEL();
    BASE_MODEL(int *start);
    void reset();
    void reset(int *start);
    inline int encodeSymbol(RangeCoder *rc, uint sym);
    inline void updateSymbol(uint sym);
    inline uint decodeSymbol(RangeCoder *rc);
    inline uint getTopSym(void);
    inline uint getSummFreq(void);

protected:
    void   rescaleRare();

    st_t SummFreq;
    st_t Stats[4];
};

template <typename st_t>
BASE_MODEL<st_t>::BASE_MODEL()
{
    reset();
}

template <typename st_t>
BASE_MODEL<st_t>::BASE_MODEL(int *start) {
    SummFreq = 0;
    for (int i = 0; i < 4; i++) {
	Stats[i] =  start[i];
	SummFreq += start[i];
    }
}

template <typename st_t>
void BASE_MODEL<st_t>::reset() {
    SummFreq = 0;

    for ( int i=0; i<4; i++ )
	SummFreq += (Stats[i] = 3*STEP);
}

template <typename st_t>
void BASE_MODEL<st_t>::reset(int *start) {
    SummFreq = 0;
    for (int i = 0; i < 4; i++) {
	Stats[i] =  start[i];
	SummFreq += start[i];
    }
}

template <typename st_t>
void BASE_MODEL<st_t>::rescaleRare()
{
    if ((Stats[0] >>= 1) == 0) Stats[0] = 1;
    if ((Stats[1] >>= 1) == 0) Stats[1] = 1;
    if ((Stats[2] >>= 1) == 0) Stats[2] = 1;
    if ((Stats[3] >>= 1) == 0) Stats[3] = 1;
    SummFreq = Stats[0] + Stats[1] + Stats[2] + Stats[3];
}

template <typename st_t>
inline int BASE_MODEL<st_t>::encodeSymbol(RangeCoder *rc, uint sym) {
    if ( SummFreq>=WSIZ ) rescaleRare();

#if 1
    if (sym == 4) {
	/* N => pick most likely instead */
	int r = 0;
	sym = 0;
	for (int i = 0; i < 4; i++) {
	    if (r < Stats[i]) {
		r = Stats[i];
		sym = i;
	    }
	}
    }
#endif
    assert(SummFreq < WSIZ);
    assert(SummFreq == Stats[0] + Stats[1] + Stats[2] + Stats[3]);

    switch(sym) {
    case 0:
	rc->Encode(0,                              Stats[0], SummFreq);
	Stats[0] += STEP; 
	SummFreq += STEP;
	return 0;
    case 1:
	rc->Encode(Stats[0],                       Stats[1], SummFreq);
	Stats[1] += STEP;
	SummFreq += STEP;
	return 1;
    case 2:
	rc->Encode(Stats[0] + Stats[1],            Stats[2], SummFreq);
	Stats[2] += STEP;
	SummFreq += STEP;
	return 2;
    case 3:
	rc->Encode(Stats[0] + Stats[1] + Stats[2], Stats[3], SummFreq);
	Stats[3] += STEP;
	SummFreq += STEP;
	return 3;
    }

    return 0;
}

template <typename st_t>
inline void BASE_MODEL<st_t>::updateSymbol(uint sym) {
  if ( SummFreq>=WSIZ ) rescaleRare();

  if (sym == 4)
      return;

  /* known symbol */
  Stats[sym] += STEP;            
  SummFreq   += STEP;
}

/*
 * Returns the bias of the best symbol compared to all other symbols.
 * This is a measure of how well adapted this model thinks it is to the
 * incoming probabilities.
 */
template <typename st_t>
inline uint BASE_MODEL<st_t>::getTopSym(void) {
    return M4(Stats);
}

template <typename st_t>
inline uint BASE_MODEL<st_t>::getSummFreq(void) {
    return SummFreq;
}

template <typename st_t>
inline uint BASE_MODEL<st_t>::decodeSymbol(RangeCoder *rc) {
    if ( SummFreq>=WSIZ) rescaleRare();

    uint count=rc->GetFreq(SummFreq);
    uint HiCount=0;             

    assert(SummFreq < WSIZ);
    assert(SummFreq == Stats[0] + Stats[1] + Stats[2] + Stats[3]);

    st_t* p=Stats;
    if ((HiCount += *p) > count) {
	rc->Decode(0, *p, SummFreq);
	Stats[0] += STEP;
	SummFreq += STEP;
	return 0;
    }

    if ((HiCount += *++p) > count) {
	rc->Decode(HiCount-*p, *p, SummFreq);
	Stats[1] += STEP;
	SummFreq += STEP;
	return 1;
    }

    if ((HiCount += *++p) > count) {
	rc->Decode(HiCount-*p, *p, SummFreq);
	Stats[2] += STEP;
	SummFreq += STEP;
	return 2;
    }

    rc->Decode(HiCount, Stats[3], SummFreq);
    Stats[3] += STEP;
    SummFreq += STEP;
    return 3;
}

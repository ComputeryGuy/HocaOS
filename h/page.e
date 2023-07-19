extern pageinit();
extern getfreeframe();
extern pagein();
extern putframe();



extern int sem_pf;
extern int pf_ctr, pf_start;
extern int Tsysframe[5];
extern int Tmmframe[5];
extern int Scronframe, Spagedframe, Sdiskframe;
extern int Tsysstack[5];
extern int Tmmstack[5];
extern int Scronstack, Spagedstack, Sdiskstack;

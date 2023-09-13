/*
>>>> Q_SER.H     >>> VERSIONE CON INTERFACCIA SERIALE LORIS <<<

++++              File include per SER.CPP
++++            Modulo di automazione QUADRA.               ++++
++++      (C) L.C.M. di Claudio Arrighi - Carrara           ++++
++++                 Italy 1995/96/97                       ++++
++++           Tutti i diritti sono riservati.              ++++
++++    Sviluppo : Walter Moretti - Carrara - Italy 1997    ++++

*/


void comm_init(void);
void comm_end(void);
void cnvex(unsigned int d_num, int n_bytes, char *ex_num);
int cpu_cmd(int l_comando, char *comando);
int com_chk(void);
int GetCPUVersion( int& ver, int& rev );
int GetMachineTemperature( float& temp );

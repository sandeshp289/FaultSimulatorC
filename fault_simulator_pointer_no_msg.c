////////////////////////////////////////////////////////////////////////
// File: Logic Simulator  Author: Sandesh Prabhakar
////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum
{
   G_JUNK,         /* 0 */
   G_INPUT,        /* 1 */
   G_OUTPUT,       /* 2 */
   G_XOR,          /* 3 */
   G_XNOR,         /* 4 */
   G_DFF,          /* 5 */
   G_AND,          /* 6 */
   G_NAND,         /* 7 */
   G_OR,           /* 8 */
   G_NOR,          /* 9 */
   G_NOT,          /* 10 */
   G_BUF,          /* 11 */
   G_TIE0,         /* 12 */
   G_TIE1,         /* 13 */
};

struct node
{

//for the gates
        char value; 
        bool flag;
//for the event list
        int data;
        node *link;
};

////////////////////////////////////////////////////////////////////////
// circuit class
////////////////////////////////////////////////////////////////////////

class circuit
{
    // circuit information
    int numgates;       // total number of gates (faulty included)
    int maxlevels;      // number of levels in gate level ckt
    int numInputs;	// number of inputs
    int numOutputs;	// number of outputs
    unsigned char *gtype;	// gate type
    short *fanin;		// number of fanins of gate
    short *fanout;		// number of fanouts of gate
    int *levelNum;		// level number of gate
    int **faninlist;		// fanin list of gate
    int **faninlist_copy;		// fanin list of gate
    int **fanoutlist;		// fanout list of gate

    int **FL_UNCOL_IN;
    int **FL_UNCOL_OUT;
    int **FL_COL_IN;
    int **FL_COL_OUT;

    char **store_input;
    char **store_output;
    char **store_dff;
    char *store_final_next_state;

    // you may wish to add more private variables here
    
    node *gate;
    node **eventlist;
    char *pattern_buffer;
    int *pout, *dff;
    int numDFFs;

    public:
    circuit(char *);            // constructor

    // you may wish to add more function declarations (or public variables) here
    void add_event (int gate_number, int level_number ); 
    void logic_simulate(int level_number, node *gate, char *pattern_buffer, int* levelNum, short* fanin, int** faninlist, short* fanout, int** fanoutlist, unsigned char* gtype);
    void evaluate(int j, unsigned char* gtype, node* gate, char* pattern_buffer, short* fanin, int** faninlist );
    void collapse_faults(int** FL_UNCOL_IN, int**FL_UNCOL_OUT, int** FL_COL_IN, int** FL_COL_OUT, short int* fanin, unsigned char* gtype, int gatenum);
    int fault_simulate(int total_patterns, int total_levels, node *gate, char *pattern_buffer, int* levelNum, short* fanin, int** faninlist, short* fanout, int** fanoutlist, unsigned char* gtype, char** store_input, char** store_output, char** store_dff, char* store_final_next_state);

};

////////////////////////////////////////////////////////////////////////
// constructor: reads in the *.lev file and builds basic data structure
// 		for the gate-level ckt
////////////////////////////////////////////////////////////////////////
circuit::circuit(char *cktName)
{
    FILE *inFile;
    char fName[40];
    int i, j, count;
    char c;
    int gatenum, junk;
    int f1, f2, f3;

    int lines, bit_count, vec_junk;

    strcpy(fName, cktName);
    strcat(fName, ".lev");
    inFile = fopen(fName, "r");
    if (inFile == NULL)
    {
	fprintf(stderr, "Can't open .lev file\n");
	exit(-1);
    }

    numgates = maxlevels = 0;
    numInputs = numOutputs = 0;

    fscanf(inFile, "%d", &count);	// number of gates
    fscanf(inFile, "%d", &junk);	// skip the second line

    // allocate space for gates data structure
    gtype = new unsigned char[count+2];
    fanin = new short[count];
    fanout = new short[count];
    levelNum = new int[count];
    faninlist = new int * [count];
    faninlist_copy = new int * [count];
    fanoutlist = new int * [count];

    FL_UNCOL_IN = new int * [count];
    FL_UNCOL_OUT = new int * [count];
    FL_COL_IN = new int * [count];
    FL_COL_OUT = new int * [count];

    for (i=1;i<count;i++)
    {
    FL_UNCOL_IN[i] = new int[2];
    FL_UNCOL_OUT[i] = new int[2];
    FL_COL_IN[i] = new int[2];
    FL_COL_OUT[i] = new int[2];
       for(j=0;j<2;j++)
        {
         FL_UNCOL_IN[i][j] = 0;
         FL_UNCOL_OUT[i][j] = 0;
         FL_COL_IN[i][j] = 0;
         FL_COL_OUT[i][j] = 0;
        }
    }

    gate = new node[count+2];
    pout = new int[count];
    dff = new int[count];

    // now read in the circuit
    for (i=1; i<count; i++)
    {

	fscanf(inFile, "%d", &gatenum);
	fscanf(inFile, "%d", &f1);
	fscanf(inFile, "%d", &f2);
	fscanf(inFile, "%d", &f3);

	numgates++;
	gtype[gatenum] = (unsigned char) f1;
	if (gtype[gatenum] > 13)
	    printf("gate %d is an unimplemented gate type\n", gatenum);
	else if (gtype[gatenum] == G_INPUT)
	    numInputs++;
	else if (gtype[gatenum] == G_OUTPUT)
            {
	    numOutputs++;
            pout[numOutputs]=gatenum; 
	    }
        else if (gtype[gatenum] == G_DFF)
            {
            numDFFs++;
            dff[numDFFs]=gatenum;  
            }


	f2 = (int) f2;
        //levelNum[gatenum] = f2;
        levelNum[gatenum] = (f2+5)/5;	


	if (f2 >= (maxlevels))
	    maxlevels = f2 + 5;

        //initializing the output of all gates to X and flags to 0
        gate[gatenum].value = 'X';
        gate[gatenum].flag = 0; 

	fanin[gatenum] = (int) f3;
	// now read in the faninlist
	faninlist[gatenum] = new int[fanin[gatenum]];
	faninlist_copy[gatenum] = new int[fanin[gatenum]];
	for (j=0; j<fanin[gatenum]; j++)
	{
	    fscanf(inFile, "%d", &f1);
	    faninlist[gatenum][j] = (int) f1;
	    faninlist_copy[gatenum][j] = (int) f1;
	}

	for (j=0; j<fanin[gatenum]; j++) // followed by samethings
	    fscanf(inFile, "%d", &junk);

	// read in the fanout list
	fscanf(inFile, "%d", &f1);
	fanout[gatenum] = (int) f1;

	fanoutlist[gatenum] = new int[fanout[gatenum]];
	for (j=0; j<fanout[gatenum]; j++)
	{
	    fscanf(inFile, "%d", &f1);
	    fanoutlist[gatenum][j] = (int) f1;
	}

        collapse_faults(FL_UNCOL_IN,FL_UNCOL_OUT,FL_COL_IN,FL_COL_OUT,fanin,gtype,gatenum);

	// skip till end of line
	while ((c = getc(inFile)) != '\n' && c != EOF)
	    ;
    }	// for (i...)
    fclose(inFile);

    gtype[count]= (unsigned char) 12;   //TIE0 for injecting s-a-0
    gtype[count+1]= (unsigned char) 13; //TIE1 for injecting s-a-1
    gate[count].value = '0';
    gate[count].flag = 0; 
    gate[count+1].value = '1';
    gate[count+1].flag = 0; 


    int COL_faults_count=0;
    int UNCOL_faults_count=0;
    for(i=1;i<count;i++)
       {
       UNCOL_faults_count=UNCOL_faults_count+FL_UNCOL_IN[i][0]+FL_UNCOL_IN[i][1]+FL_UNCOL_OUT[i][0]+FL_UNCOL_OUT[i][1];
       COL_faults_count=COL_faults_count+FL_COL_IN[i][0]+FL_COL_IN[i][1]+FL_COL_OUT[i][0]+FL_COL_OUT[i][1];
       }

//PRINTING COLLAPSED FAULT LIST
printf("\nCOLLAPSED FAULT LIST\n");
    for(i=1;i<count;i++)
       {
       for(j=0;j<FL_COL_IN[i][0];j++)
         {
         printf("GATE %d INP %d s-a-0\n",i,j); 
         }
       for(j=0;j<FL_COL_IN[i][1];j++)
         {
         printf("GATE %d INP %d s-a-1\n",i,j); 
         }
       for(j=0;j<FL_COL_OUT[i][0];j++)
         {
         printf("GATE %d OUT s-a-0\n",i); 
         }
       for(j=0;j<FL_COL_OUT[i][1];j++)
         {
         printf("GATE %d OUT s-a-1\n",i); 
         }
       }

    int total_levels = maxlevels/5; 

    printf("Successfully read in circuit:\n");
    printf("\t%d PIs.\n", numInputs);
    printf("\t%d POs.\n", numOutputs);
    printf("\t%d Dffs.\n", numDFFs);
    printf("\t%d total number of gates\n", numgates);
    printf("\t%d levels in the circuit.\n", total_levels);
    printf("\t%d Total Number of Uncollapsed Faults \n",UNCOL_faults_count );
    printf("\t%d Total Number of Collapsed Faults \n",COL_faults_count );


/*store_input = new char[1000][numInputs+2];
store_output = new char[1000][numOutputs+2];
  if(numDFFs>0)
    {
    store_dff = new char[1000][numDFFs+2];
    store_final_next_state = new char[numDFFs+2];
    }
*/
store_input = new char *[10000];
store_output = new char * [10000];

    for(i=0;i<10000;i++)
        {
          store_input[i] = new char [numInputs+2];
          store_output[i] = new char [numOutputs+2];
        }
  if(numDFFs>0)
    {
    store_dff = new char * [10000];
    for(i=0;i<10000;i++)
        {
          store_dff[i] = new char [numDFFs+2];
        }
    store_final_next_state = new char[numDFFs+2];
    }

    // (LOGIC SIMULATOR CODE STARTS HERE)
    
    // allocate space for event list and pattern buffer
    eventlist = new node*[total_levels+1];

    for(i=0;i<total_levels+1;i++)
    eventlist[i]=NULL;

    pattern_buffer = new char[numInputs+2];

    strcpy(fName, cktName);
	strcat(fName, ".vec");
	inFile = fopen(fName, "r");
	if (inFile == NULL)
	  {
		fprintf(stderr, "Can't open .vec file\n");
		exit(-1);
	  }

        lines = 0;
        vec_junk = 's';

	fscanf(inFile, "%d", &bit_count);	// number of bits in each vector

        while (vec_junk!='\n')
	fscanf(inFile, "%c", &vec_junk);	// reading space and newline char

        //For each input vector
    	while ((c = getc(inFile)) != 'E')
	  {
               lines = lines + 1;
            
	       pattern_buffer[1]=c;
               printf("\n \n");


	       //Reading the input vector
	       printf("Input Vector  %d:     %c", lines-1, pattern_buffer[1]);
               store_input[lines][1]=pattern_buffer[1];
               if(gate[1].value != pattern_buffer[1])
               {
               add_event(1,1); 
               gate[1].flag = 1;
               }

	       for (i=2; i<=bit_count; i++)
	       {
	         fscanf(inFile, "%c", &pattern_buffer[i]);
		 printf("%c", pattern_buffer[i]);
                 store_input[lines][i]=pattern_buffer[i];
                   if(gate[i].value != pattern_buffer[i])
                     { 
                      add_event(i,1); 
                      gate[i].flag = 1;
                     }
               } 

               vec_junk = 's';
               while (vec_junk!='\n') 
               fscanf(inFile, "%c", &vec_junk);

               //For each level
               for(i=1;i<=total_levels;i++)            
               {
                 //Calling the Logic Simulate function
                 logic_simulate(i, gate, pattern_buffer, levelNum, fanin, faninlist, fanout, fanoutlist, gtype);
               }

               //Output Vector
               printf("\nOutput Vector %d:     ", lines-1);
               for(i=1;i<=numOutputs;i++)
                {
                 printf("%c",gate[pout[i]].value);
                 store_output[lines][i]=gate[pout[i]].value;
                }

               //Next state for sequential circuits 
	       printf("\n"); 
               if(numDFFs>0)
               {
               for(i=1;i<=numDFFs;i++)
                 {
                 printf("%c",gate[dff[i]].value);
                 store_dff[lines][i]=gate[dff[i]].value;
                 }
               }

           }//While loop ends
               fclose(inFile);
               //Final next state for sequential circuits
               if(numDFFs>0)
               {
               printf("\n \nFinal Next State = ");
               for(i=1;i<=numDFFs;i++)
                {
                 printf("%c",gate[faninlist[dff[i]][0]].value);
                 store_final_next_state[i]=gate[faninlist[dff[i]][0]].value;
                }
               }
               printf("\n \nTotal number of patterns is %d\n", lines);


/*   printf("\n VERIFYING....\n");
               printf("\n \nINPUT_PATTERN\n");
               for(i=1;i<=lines;i++)
                {
                for(j=1;j<=numInputs;j++)
                  {
                  printf("%c",store_input[i][j]);
                  }
                 printf("\n");
                }

               printf("\n \nOUTPUT_PATTERN\n");
               for(i=1;i<=lines;i++)
                {
                for(j=1;j<=numOutputs;j++)
                  {
                  printf("%c",store_output[i][j]);
                  }
                 printf("\n");
                if(numDFFs>0)
                  {
                    for(j=1;j<=numDFFs;j++)
                     {
                      printf("%c",store_dff[i][j]);
                     }
                 printf("\n");
                  }
                }


              if(numDFFs>0)
               {
               printf("\n \nFINAL NEXT STATE\n ");

               for(i=1;i<=numDFFs;i++)
                {
                 printf("%c",store_final_next_state[i]);
                }
                 printf("\n");
               }
*/

//FAULT SIMULATOR
int fs,l,dcount,dflag,total_patterns,temp,temp1,temp2;
dcount=0;
dflag=0;
total_patterns=lines;

            int store_temp1[10000];
            int store_temp2[10000];
            int z=0;

for(fs=1;fs<count;fs++)
  {
         //FL_COL_IN[fs][0]
       for(l=0;l<FL_COL_IN[fs][0];l++)  
          {
             printf("\nInjecting fault: GATE %d INP %d s-a-0\n",fs,l);
             for(i=0;i<total_levels+1;i++)
             eventlist[i]=NULL;

             for(i=1;i<count;i++)
               {
                gate[i].value = 'X';
                gate[i].flag = 0; 
               }
             temp=faninlist[fs][l];
             faninlist[fs][l]=count;
             add_event(fs,levelNum[fs]);
             gate[fs].flag=1;

            z=0;
            for(i=0;i<fanout[temp];i++)
             {
               temp1=fanoutlist[temp][i];
               store_temp2[i]=temp1;
                      if(fs!=temp1)
                        {
                        store_temp1[z]=temp1;
                        z++;
                        }
             }
            fanout[temp]=fanout[temp]-1;
            for(i=0;i<fanout[temp];i++)
             {
               fanoutlist[temp][i]=store_temp1[i];
             }


             dflag=fault_simulate(total_patterns, total_levels, gate, pattern_buffer, levelNum, fanin, faninlist, fanout, fanoutlist, gtype, store_input, store_output, store_dff, store_final_next_state);
                if(dflag==1)
                 {
                 printf("Fault GATE %d INP %d s-a-0 detected\n",fs,l);
                 dcount++;
                 }
                 else
                 {
                 printf("Fault GATE %d INP %d s-a-0 not detected\n",fs,l);
                 }

             faninlist[fs][l]=temp;
            fanout[temp]=fanout[temp]+1;
            for(i=0;i<fanout[temp];i++)
             {
               fanoutlist[temp][i]=store_temp2[i];
             }

          }       


         //FL_COL_IN[fs][1]
       for(l=0;l<FL_COL_IN[fs][1];l++)  
          {
             printf("\nInjecting fault: GATE %d INP %d s-a-1\n",fs,l);

             for(i=0;i<total_levels+1;i++)
             eventlist[i]=NULL;

             for(i=1;i<count;i++)
               {
                gate[i].value = 'X';
                gate[i].flag = 0; 
               }
             temp=faninlist[fs][l];
             faninlist[fs][l]=count+1;
             add_event(fs,levelNum[fs]);
             gate[fs].flag=1;

            z=0;
            for(i=0;i<fanout[temp];i++)
             {
               temp1=fanoutlist[temp][i];
               store_temp2[i]=temp1;
                      if(fs!=temp1)
                        {
                        store_temp1[z]=temp1;
                        z++;
                        }
             }
            fanout[temp]=fanout[temp]-1;
            for(i=0;i<fanout[temp];i++)
             {
               fanoutlist[temp][i]=store_temp1[i];
             }


             dflag=fault_simulate(total_patterns, total_levels, gate, pattern_buffer, levelNum, fanin, faninlist, fanout, fanoutlist, gtype, store_input, store_output, store_dff, store_final_next_state);

                if(dflag==1)
                 {
                 printf("Fault GATE %d INP %d s-a-1 detected\n",fs,l);
                 dcount++;
                 }
                 else
                 {
                 printf("Fault GATE %d INP %d s-a-1 not detected\n",fs,l);
                 }

             faninlist[fs][l]=temp;
            fanout[temp]=fanout[temp]+1;
            for(i=0;i<fanout[temp];i++)
             {
               fanoutlist[temp][i]=store_temp2[i];
             }
        } 

         //FL_COL_OUT[fs][0]
         for(l=0;l<FL_COL_OUT[fs][0];l++)  
           {
             printf("\nInjecting fault: GATE %d OUT s-a-0\n",fs);

             for(i=0;i<total_levels+1;i++)
             eventlist[i]=NULL;

             for(i=1;i<count;i++)
               {
                gate[i].value = 'X';
                gate[i].flag = 0; 
               }

            for(i=0;i<fanout[fs];i++)
             {
               temp1=fanoutlist[fs][i];
                   for(j=0;j<fanin[temp1];j++)
                     {
                      if(fs==faninlist[temp1][j])
                        {
                        temp=faninlist[temp1][j];
                        faninlist[temp1][j]=count;
                        add_event(temp1,levelNum[temp1]);
                        gate[temp1].flag=1;
                        }
                     }
             }

            temp2=fanout[fs];
            fanout[fs]=0;

             dflag=fault_simulate(total_patterns, total_levels, gate, pattern_buffer, levelNum, fanin, faninlist, fanout, fanoutlist, gtype, store_input, store_output, store_dff, store_final_next_state);

                if(dflag==1)
                 {
                 printf("Fault GATE %d OUT s-a-0 detected\n",fs);
                 dcount++;
                 }
                 else
                 {
                 printf("Fault GATE %d OUT s-a-0 not detected\n",fs,l);
                 }


             fanout[fs]=temp2;

             for(i=0;i<fanout[fs];i++)
               {
                temp1=fanoutlist[fs][i];
                  for(j=0;j<fanin[temp1];j++)
                   {
                    faninlist[temp1][j]=faninlist_copy[temp1][j];
                   }
               }


         }

       //FL_COL_OUT[fs][1]
       for(l=0;l<FL_COL_OUT[fs][1];l++)  
          {
             printf("\nInjecting fault: GATE %d OUT s-a-1\n",fs);

             for(i=0;i<total_levels+1;i++)
             eventlist[i]=NULL;

             for(i=1;i<count;i++)
               {
                gate[i].value = 'X';
                gate[i].flag = 0; 
               }

            for(i=0;i<fanout[fs];i++)
             {
               temp1=fanoutlist[fs][i];
                   for(j=0;j<fanin[temp1];j++)
                     {
                      if(fs==faninlist[temp1][j])
                        {
                        temp=faninlist[temp1][j];
                        faninlist[temp1][j]=count+1;
                        add_event(temp1,levelNum[temp1]);
                        gate[temp1].flag=1;
                        }
                     }
             }

            temp2=fanout[fs];
            fanout[fs]=0;

             dflag=fault_simulate(total_patterns, total_levels, gate, pattern_buffer, levelNum, fanin, faninlist, fanout, fanoutlist, gtype, store_input, store_output, store_dff, store_final_next_state);

                if(dflag==1)
                 {
                 printf("Fault GATE %d OUT s-a-1 detected\n",fs);
                 dcount++;
                 }
                 else
                 {
                 printf("Fault GATE %d OUT s-a-1 not detected\n",fs,l);
                 }

             fanout[fs]=temp2;

             for(i=0;i<fanout[fs];i++)
               {
                temp1=fanoutlist[fs][i];
                  for(j=0;j<fanin[temp1];j++)
                   {
                    faninlist[temp1][j]=faninlist_copy[temp1][j];
                   }
               }

          }       

   }
    printf("\nTotal Number of Collapsed Faults=%d\n",COL_faults_count );
    printf("\nTotal Detected Faults=%d\n",dcount);


}//Constructor ends

////////////////////////////////////////////////////////////////////////
// other member functions here
////////////////////////////////////////////////////////////////////////

//Function to schedule gate onto the eventwheel
void circuit::add_event (int gate_number, int level_number )
 	{
	node *current = new node;
	current->data =gate_number; 
	current->link = eventlist[level_number];
        eventlist[level_number] = current;
        }

//Logic simulator function
void circuit::logic_simulate(int level_number, node *gate, char *pattern_buffer, int* levelNum, short* fanin, int** faninlist, short* fanout, int** fanoutlist, unsigned char* gtype)
        {
           node *delNode = new node;
           char temp;
           int i, num;
           while(eventlist[level_number] != NULL)
	   {
           num = eventlist[level_number]->data;
           temp = gate[num].value;
           evaluate(num, gtype, gate, pattern_buffer, fanin, faninlist );
           gate[num].flag = 0;

           if((temp != gate[num].value)&&gtype[num]!=2)
                    for(i=0; i<fanout[num]; i++)
                    { 
                      if(gate[fanoutlist[num][i]].flag == 0)
		      { 
                      add_event(fanoutlist[num][i], levelNum[fanoutlist[num][i]]); 
                      gate[fanoutlist[num][i]].flag = 1;
                      }
                    }

	   delNode = eventlist[level_number];
	   eventlist[level_number] = eventlist[level_number]->link;
	   delete delNode;
	   }
        }

       int circuit::fault_simulate(int total_patterns, int total_levels, node *gate, char *pattern_buffer, int* levelNum, short* fanin, int** faninlist, short* fanout, int** fanoutlist, unsigned char* gtype, char** store_input, char** store_output, char** store_dff, char* store_final_next_state)
         {
           int i,j,k;
           int detected_flag=0;

            for(i=1;i<=total_patterns;i++)	
	      {
            
                        //printf("Input Vector  %d: ",i-1);
	       		for (j=1; j<=numInputs; j++)
	      			 {
                                 //printf("%c",store_input[i][j]);
	       			  pattern_buffer[j]=store_input[i][j];
                 			  if(gate[j].value != pattern_buffer[j])
                   				  { 
                     					 add_event(j,1); 
                     					 gate[j].flag = 1;
                     				  }
               			 } 
                                 //printf("\n");

               		//For each level
              		for(k=1;k<=total_levels;k++)            
              			 {
                 		//Calling the Logic Simulate function
                 		logic_simulate(k, gate, pattern_buffer, levelNum, fanin, faninlist, fanout, fanoutlist, gtype);
              			 }

               		//Output Vector
                        //printf("Fault-Free Output Vector %d: ",i-1);
                            //for(k=1;k<=numOutputs;k++)
              			  //{
               			  //printf("%c",store_output[i][k]);
               			  //} 		 
                                  //printf("\n");
                        //printf("Faulty Output Vector %d: ",i-1);
				for(k=1;k<=numOutputs;k++)
               			 {
               			  //printf("%c",gate[pout[k]].value);
                			 if((store_output[i][k] != gate[pout[k]].value)&&(store_output[i][k]!='X')&&(gate[pout[k]].value!='X'))
                 				  {
     									//printf("\nFault-Free value O %c\n",store_output[i][k]);
                                                                        //printf("\nFaulty value O %c\n",gate[pout[k]].value);
                 					                detected_flag=1;
                  				  }
               			 }
                                  //printf("\n");


               		if(numDFFs>0&&detected_flag==0)
             			  {
                                         //printf("Fault-Free DFF values %d: ",i-1);
                                         
              				 //for(k=1;k<=numDFFs;k++)
                				 //{
                				 //printf("%c",store_dff[i][k]);
                				 //}
                                         //printf("\n");
                                         //printf("Faulty DFF values %d: ",i-1);
              				 for(k=1;k<=numDFFs;k++)
                				 {
                				          //printf("%c",gate[dff[k]].value);
                					  if((store_dff[i][k] != gate[dff[k]].value)&&(store_dff[i][k]!='X')&&(gate[dff[k]].value!='X'))
                  						  {
                                                                         //printf("\nFault-Free value D %c\n",store_dff[i][k]);
                                                                         //printf("\nFaulty value D %c\n",gate[dff[k]].value);
                   							 detected_flag=1;
                    						  }
                				 }
                                         //printf("\n");
              			  }

           		if(detected_flag==1)
                        //printf("FAULT DETECTED\n");

           		if(detected_flag==1)
           	        break;

              }//For loop ends

               //Final next state for sequential circuits
           /*  if(numDFFs>0 && detected_flag==0)
               {
            	   for(k=1;k<=numDFFs;k++)
                	{
                 		if((store_final_next_state[i] != gate[faninlist[dff[k]][0]].value)&&(store_final_next_state[k]!='X')&&(gate[faninlist[dff[k]][0]].value!='X'))
                  		{
                     		detected_flag=1;
                 		 }
                	}
               }*/
          return detected_flag;
        } //function ends

void circuit::collapse_faults(int** FL_UNCOL_IN, int**FL_UNCOL_OUT, int** FL_COL_IN, int** FL_COL_OUT, short int* fanin, unsigned char* gtype, int gatenum)
   {

         switch (gtype[gatenum])
              { //switch begins
			 //INPUT
		          case 1:
		          FL_UNCOL_IN[gatenum][0]=0;
		          FL_UNCOL_OUT[gatenum][0]=1;
		          FL_UNCOL_IN[gatenum][1]=0;
		          FL_UNCOL_OUT[gatenum][1]=1;

		          FL_COL_IN[gatenum][0]=0;
		          FL_COL_OUT[gatenum][0]=1;
		          FL_COL_IN[gatenum][1]=0;
		          FL_COL_OUT[gatenum][1]=1;
		          break;

                          //NAND gate
			  case 7:
		          FL_UNCOL_IN[gatenum][0]=fanin[gatenum];
		          FL_UNCOL_OUT[gatenum][0]=1;
		          FL_UNCOL_IN[gatenum][1]=fanin[gatenum];
		          FL_UNCOL_OUT[gatenum][1]=1;

		          FL_COL_IN[gatenum][0]=0;
		          FL_COL_OUT[gatenum][0]=1;
		          FL_COL_IN[gatenum][1]=fanin[gatenum];
		          FL_COL_OUT[gatenum][1]=1;
			  break;

                          //AND gate
                          case 6:
		          FL_UNCOL_IN[gatenum][0]=fanin[gatenum];
		          FL_UNCOL_OUT[gatenum][0]=1;
		          FL_UNCOL_IN[gatenum][1]=fanin[gatenum];
		          FL_UNCOL_OUT[gatenum][1]=1;

		          FL_COL_IN[gatenum][0]=0;
		          FL_COL_OUT[gatenum][0]=1;
		          FL_COL_IN[gatenum][1]=fanin[gatenum];
		          FL_COL_OUT[gatenum][1]=1;
                          break;

                          //OR gate
                          case 8:
		          FL_UNCOL_IN[gatenum][0]=fanin[gatenum];
		          FL_UNCOL_OUT[gatenum][0]=1;
		          FL_UNCOL_IN[gatenum][1]=fanin[gatenum];
		          FL_UNCOL_OUT[gatenum][1]=1;

		          FL_COL_IN[gatenum][0]=fanin[gatenum];
		          FL_COL_OUT[gatenum][0]=1;
		          FL_COL_IN[gatenum][1]=0;
		          FL_COL_OUT[gatenum][1]=1;
                          break;

                          //NOR gate
                          case 9:
		          FL_UNCOL_IN[gatenum][0]=fanin[gatenum];
		          FL_UNCOL_OUT[gatenum][0]=1;
		          FL_UNCOL_IN[gatenum][1]=fanin[gatenum];
		          FL_UNCOL_OUT[gatenum][1]=1;

		          FL_COL_IN[gatenum][0]=fanin[gatenum];
		          FL_COL_OUT[gatenum][0]=1;
		          FL_COL_IN[gatenum][1]=0;
		          FL_COL_OUT[gatenum][1]=1;
                          break;

                          //XOR gate
			  case 3:
		          FL_UNCOL_IN[gatenum][0]=fanin[gatenum];
		          FL_UNCOL_OUT[gatenum][0]=1;
		          FL_UNCOL_IN[gatenum][1]=fanin[gatenum];
		          FL_UNCOL_OUT[gatenum][1]=1;

		          FL_COL_IN[gatenum][0]=fanin[gatenum];
		          FL_COL_OUT[gatenum][0]=1;
		          FL_COL_IN[gatenum][1]=fanin[gatenum];
		          FL_COL_OUT[gatenum][1]=1;
                          break;

                          //XNOR gate
			  case 4:
		          FL_UNCOL_IN[gatenum][0]=fanin[gatenum];
		          FL_UNCOL_OUT[gatenum][0]=1;
		          FL_UNCOL_IN[gatenum][1]=fanin[gatenum];
		          FL_UNCOL_OUT[gatenum][1]=1;

		          FL_COL_IN[gatenum][0]=fanin[gatenum];
		          FL_COL_OUT[gatenum][0]=1;
		          FL_COL_IN[gatenum][1]=fanin[gatenum];
		          FL_COL_OUT[gatenum][1]=1;
                          break;

			  //NOT gate
                          case 10:
		          FL_UNCOL_IN[gatenum][0]=1;
		          FL_UNCOL_OUT[gatenum][0]=1;
		          FL_UNCOL_IN[gatenum][1]=1;
		          FL_UNCOL_OUT[gatenum][1]=1;

		          FL_COL_IN[gatenum][0]=0;
		          FL_COL_OUT[gatenum][0]=1;
		          FL_COL_IN[gatenum][1]=0;
		          FL_COL_OUT[gatenum][1]=1;
                          break;

                          //BUF gate
			  case 11:
		          FL_UNCOL_IN[gatenum][0]=1;
		          FL_UNCOL_OUT[gatenum][0]=1;
		          FL_UNCOL_IN[gatenum][1]=1;
		          FL_UNCOL_OUT[gatenum][1]=1;

		          FL_COL_IN[gatenum][0]=0;
		          FL_COL_OUT[gatenum][0]=1;
		          FL_COL_IN[gatenum][1]=0;
		          FL_COL_OUT[gatenum][1]=1;
                          break;

                          //TIE_1
                          case 12:
		          FL_UNCOL_IN[gatenum][0]=0;
		          FL_UNCOL_OUT[gatenum][0]=1;
		          FL_UNCOL_IN[gatenum][1]=0;
		          FL_UNCOL_OUT[gatenum][1]=0;

		          FL_COL_IN[gatenum][0]=0;
		          FL_COL_OUT[gatenum][0]=1;
		          FL_COL_IN[gatenum][1]=0;
		          FL_COL_OUT[gatenum][1]=0;
		          break;

                          //TIE_0
			  case 13:
		          FL_UNCOL_IN[gatenum][0]=0;
		          FL_UNCOL_OUT[gatenum][0]=0;
		          FL_UNCOL_IN[gatenum][1]=0;
		          FL_UNCOL_OUT[gatenum][1]=1;

		          FL_COL_IN[gatenum][0]=0;
		          FL_COL_OUT[gatenum][0]=0;
		          FL_COL_IN[gatenum][1]=0;
		          FL_COL_OUT[gatenum][1]=1;
                          break;

                          //OUTPUT
			  case 2:
		          FL_UNCOL_IN[gatenum][0]=1;
		          FL_UNCOL_OUT[gatenum][0]=0;
		          FL_UNCOL_IN[gatenum][1]=1;
		          FL_UNCOL_OUT[gatenum][1]=0;

		          FL_COL_IN[gatenum][0]=1;
		          FL_COL_OUT[gatenum][0]=0;
		          FL_COL_IN[gatenum][1]=1;
		          FL_COL_OUT[gatenum][1]=0;
                          break;

			  //D_FF
			  case 5:
		          FL_UNCOL_IN[gatenum][0]=1;
		          FL_UNCOL_OUT[gatenum][0]=1;
		          FL_UNCOL_IN[gatenum][1]=1;
		          FL_UNCOL_OUT[gatenum][1]=1;

		          FL_COL_IN[gatenum][0]=1;
		          FL_COL_OUT[gatenum][0]=1;
		          FL_COL_IN[gatenum][1]=1;
		          FL_COL_OUT[gatenum][1]=1;
			  break;

		   } //switch ends


		} //function ends


//Gate evaluations for logic simulator
void circuit::evaluate (int j, unsigned char* gtype, node* gate, char* pattern_buffer, short* fanin, int** faninlist )
   {
    int nn;
    char out;
    bool out_int;

         switch (gtype[j])
              { //switch begins
			 //INPUT
		          case 1:
		          gate[j].value=pattern_buffer[j];
		          break;

                          //NAND gate
			  case 7:
                          out='0';
			  for (nn=0; nn<=(fanin[j]-1);nn++)
				     {
                                        if (gate[faninlist[j][nn]].value == '0')
					 {
					 gate[j].value='1';
					 return;
                                         }
                                         else if (gate[faninlist[j][nn]].value == 'X')
                                         out='X';
				     }
                                         gate[j].value=out; 
					 break;

                              //AND gate
                              case 6:

                                     out='1';
			             for (nn=0; nn<=(fanin[j]-1);nn++)
				     {
                                        if (gate[faninlist[j][nn]].value == '0')
					 {
					 gate[j].value='0';
					 return;
                                         }
                                         else if (gate[faninlist[j][nn]].value == 'X')
                                         out='X';
				     }
                                         gate[j].value=out; 
					 break;

                              //OR gate
                              case 8:

                                     out='0';
			             for (nn=0; nn<=(fanin[j]-1);nn++)
				     {
                                        if (gate[faninlist[j][nn]].value == '1')
					 {
					 gate[j].value='1';
					 return;
                                         }
                                         else if (gate[faninlist[j][nn]].value == 'X')
                                         out='X';
				     }
                                         gate[j].value=out; 
					 break;

                              //NOR gate
                              case 9:
                                     out='1';
			             for (nn=0; nn<=(fanin[j]-1);nn++)
				     {
                                        if (gate[faninlist[j][nn]].value == '1')
					 {
					 gate[j].value='0';
					 return;
                                         }
                                         else if (gate[faninlist[j][nn]].value == 'X')
                                         out='X';
				     }
                                         gate[j].value=out; 
					 break; 

                              //XOR gate
			            case 3:
                                      out_int=0;
                                      for (nn=0; nn<=(fanin[j]-1);nn++)
				         {
                                        if (gate[faninlist[j][nn]].value == 'X')
					 {
					 gate[j].value='X';
					 return;
                                         }
                                         else if (gate[faninlist[j][nn]].value == '1')
                                         out_int=!out_int;
				        }

                                        switch (out_int)
                                            {
                                         case 0: out = '0';
                                                 break;
                                         case 1: out = '1';
                                                 break;
                                            }
                                         gate[j].value=out; 
					 break; 

                             //XNOR gate
				    case 4:
				      out_int=0;
                                      for (nn=0; nn<=(fanin[j]-1);nn++)
				         {
                                        if (gate[faninlist[j][nn]].value == 'X')
					 {
					 gate[j].value='X';
					 return;
                                         }
                                         else if (gate[faninlist[j][nn]].value == '1')
                                         out_int=!out_int;
				        }

                                        switch (out_int)
                                            {
                                         case 0: out = '1';
                                                 break;
                                         case 1: out = '0';
                                                 break;
                                            }
                                         gate[j].value=out; 
					 break; 

			      //NOT gate
                              case 10:
				        if ( gate[faninlist[j][0]].value == '0')
				  	       {
					        gate[j].value='1';
					       }
					else if (gate[faninlist[j][0]].value == '1')
					       gate[j].value='0';
					else if (gate[faninlist[j][0]].value == 'X')
						{
						gate[j].value='X';
					        }
					break;

                             //BUF gate
			      case 11:

				  	gate[j].value=gate[faninlist[j][0]].value;
					break;

                              //TIE_1
                              case 12:
					gate[j].value='1';
					break;

                              //TIE_0
			      case 13:
				        gate[j].value = '0';
				        break;

                              //OUTPUT
			      case 2:
				        gate[j].value=gate[faninlist[j][0]].value;
				        break;

			      //D_FF
			      case 5:
                                        gate[j].value=gate[faninlist[j][0]].value;
				        break;

			   } //switch ends


		} //function ends

////////////////////////////////////////////////////////////////////////
// main starts here
////////////////////////////////////////////////////////////////////////
main(int argc, char *argv[])
{
    char cktName[20];
    circuit *ckt;
    int vecWidth, poNum;

    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s <ckt>\n", argv[0]);
        exit(-1);
    }

    strcpy(cktName, argv[1]);

    // read in the circuit and build data structure
    ckt = new circuit(cktName);
    // NOW ADD YOUR FUNCTION HERE
}


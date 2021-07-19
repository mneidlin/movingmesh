/* This UDF reads the "UDFPTS" folder that has to be in the FLUENT simulation location and prescribes the movement of the nodes*/
#include "udf.h"

#define nTimeSteps 241 /* output of ps_intNPts  "Total Number of Frames after interpolation" */
#define allPtsNum 10353 /* output of ps_detNPts*/

#define Scale 0.001 /*factor was considered in udf_extPts (AssignID)*/  
#define fakeTime 0

/*Define the number of nodes per core (except for the last one) 
Values are provided by AssignID (udf_extPts) in the console, adapt from there */  
const static int PtsNum[] = { 1319, 1334, 1425, 1224, 1066, 987, 970, 699, 669, 660 } ;

static real vel, Grid_X[allPtsNum], Grid_Y[allPtsNum], Grid_Z[allPtsNum];


/*Assigns loaded node coordinates to the 3 zones (ventricle, inlet, outlet)*/
DEFINE_GRID_MOTION(Ven_move, domain, dt, time, dtime)
{
	#if !RP_HOST /* if you don’t exclude the host, this operation will result in a division by zero and error!
	Remember that host has no data so its total will be zero. */
	Thread *tf=DT_THREAD(dt);
	face_t f;
	Node *node_p;
	#endif

	real X[3];
	int nTime,n,ptID, i;
	
	nTime=(N_TIME+fakeTime)%nTimeSteps+1; /* N_TIME integer number of time steps; nTime: actual timestep in cycle
												-> restarts at 1 after one cycle */

	#if !RP_HOST
	if (!Data_Valid_P()) /* Do nothing if gradient isn’t allocated yet. */
		return;
	
	SET_DEFORMING_THREAD_FLAG(THREAD_T0(tf));
	begin_f_loop(f,tf)
	if PRINCIPAL_FACE_P(f,tf) /* tests if the face is the principle face 
								 although each face can appear on one or two partitions, it can only "officially'' belong to one of them.*/
	{
		f_node_loop(f,tf,n)
		{
			node_p=F_NODE(f,tf,n);  /* obtain the global face node number*/
			if (NODE_POS_NEED_UPDATE(node_p))
			{
			  NODE_POS_UPDATED(node_p);

			  ptID=N_UDMI(node_p,myid);

			  if (myid != 0)
			  {
			  	for (i = 0 ; i < myid ; i++)
			  	{
			  		ptID = ptID + PtsNum[i];
			  	}
			  }

			  X[0]=Grid_X[ptID];
			  X[1]=Grid_Y[ptID];
			  X[2]=Grid_Z[ptID];
			  /*Message (" \n node: %d, X: %f, Y:%f, Z: %f", ptID, X[0], X[1], X[2]);*/

			  NODE_X(node_p)=X[0]*Scale;
			  NODE_Y(node_p)=X[1]*Scale;
			  NODE_Z(node_p)=X[2]*Scale;

			}
		}
	}
	end_f_loop(f,tf)
	#endif
}

/*loads after successful timestep the new point coordinates from udfsurface file
function set at Funktion Hooks bei Define at end (Fluent environment)*/
DEFINE_EXECUTE_AT_END(loadMESH)
{
	int nTime, i;
	char nFileName[100];
	FILE *meshFrame;
	
	nTime=(N_TIME+fakeTime)%nTimeSteps+1;
	#if !RP_NODE
	sprintf(nFileName,"UDFPTS/udfsurface_%d.asc",nTime); /* string saved in nFileName  */
	meshFrame=fopen(nFileName,"r");
	Message("\n %s \n", nFileName);
	Message("\n %d \n", N_TIME);
	for (i=0; i<allPtsNum; i++)
	{
		fscanf(meshFrame,"%f %f %f", &Grid_X[i],&Grid_Y[i],&Grid_Z[i]); 
	}
	fclose(meshFrame);
	Message("Finish Loading \n");
	#endif
	host_to_node_real(Grid_X,allPtsNum);  /* integer and real variables passed from host to nodes */
	host_to_node_real(Grid_Y,allPtsNum);
	host_to_node_real(Grid_Z,allPtsNum);
}

/*Funktionhas to be run before starting simulation, performs same as loadMESH function only for first timestep. Loads current mesh. */
DEFINE_ON_DEMAND(First_loadMESH)
{
	int nTime, i;
	char nFileName[100];
	FILE *meshFrame;
	
	nTime=(N_TIME+fakeTime)%nTimeSteps+1;   /* N_TIME integer number of time steps; nTime remaining timesteps */
	#if !RP_NODE

	sprintf(nFileName,"UDFPTS/udfsurface_%d.asc",nTime);
	meshFrame=fopen(nFileName,"r");
	Message("\n %s \n", nFileName);
	Message("\n %d \n", N_TIME);
	for (i=0; i<allPtsNum; i++)
	{
		fscanf(meshFrame,"%f %f %f", &Grid_X[i],&Grid_Y[i],&Grid_Z[i]);
		/*Message (" \n Werte %d: %f %f %f \n", i, Grid_X[i], Grid_Y[i], Grid_Z[i]);*/
	}
	fclose(meshFrame);
	Message("Finish Loading \n");
	#endif
	host_to_node_real(Grid_X,allPtsNum);
	host_to_node_real(Grid_Y,allPtsNum);
	host_to_node_real(Grid_Z,allPtsNum);
}

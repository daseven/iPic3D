/***************************************************************************
 convHDF5.cpp  -  Convert program to open Parsek Output
 -------------------
 begin                : Jun 2008
 copyright            : (C) 2004 by Stefano Markidis, Giovanni Lapenta
 ************************************************************************** */

#include "hdf5.h"
#include "mpi.h"
#include "Alloc.h"
#include "math.h"
#include "Manipulator.h"
#include "VtkStuff.h"


#include <iostream>
#include <fstream>
#include <string>
#include <sstream>

int main (int argc, char **argv) {
	// cycle we want to open
		sscanf(argv[1],"%d",&InitT);
	sscanf(argv[2],"%d",&MaxLevel);
	sscanf(argv[3],"%d",&DeltaT);

	if(argc>3) {
			sscanf(argv[4], "%d", &NdimCode);
		}
		else
		{
			NdimCode = 3;
		}

    int rank, size;

   MPI_Init (&argc, &argv);	/* starts MPI */
   MPI_Comm_rank (MPI_COMM_WORLD, &rank);	/* get current process id */
   MPI_Comm_size (MPI_COMM_WORLD, &size);	/* get number of processes */
   printf( "Hello world from process %d of %d\n", rank, size );


	nlevels = MaxLevel/DeltaT;
	initlevel = InitT/DeltaT;

    int out;
	out = readsettings();
	if(out<0)
		return -1;

	nxn = nxc/XLEN;
	nyn = nyc/YLEN;
	nzn = nzc/ZLEN;


	temp_storageX = new double[(nxn+1)*(nyn+1)*(nzn+1)];
	temp_storageY = new double[(nxn+1)*(nyn+1)*(nzn+1)];
    temp_storageZ = new double[(nxn+1)*(nyn+1)*(nzn+1)];

	double*** EX = newArr3(double,nxn*XLEN,nyn*YLEN,nzn*ZLEN);
	double*** EY = newArr3(double,nxn*XLEN,nyn*YLEN,nzn*ZLEN);
	double*** EZ = newArr3(double,nxn*XLEN,nyn*YLEN,nzn*ZLEN);
	double*** BX = newArr3(double,nxn*XLEN,nyn*YLEN,nzn*ZLEN);
	double*** BY = newArr3(double,nxn*XLEN,nyn*YLEN,nzn*ZLEN);
	double*** BZ = newArr3(double,nxn*XLEN,nyn*YLEN,nzn*ZLEN);
	double*** VX = newArr3(double,nxn*XLEN,nyn*YLEN,nzn*ZLEN);
	double*** VY = newArr3(double,nxn*XLEN,nyn*YLEN,nzn*ZLEN);
	double*** VZ = newArr3(double,nxn*XLEN,nyn*YLEN,nzn*ZLEN);

	double*** NE = newArr3(double,nxn*XLEN,nyn*YLEN,nzn*ZLEN);
	double*** NI = newArr3(double,nxn*XLEN,nyn*YLEN,nzn*ZLEN);
	double*** NO = newArr3(double,nxn*XLEN,nyn*YLEN,nzn*ZLEN);
	double*** AZ = newArr3(double,nxn*XLEN,nyn*YLEN,nzn*ZLEN);

	double*** TXX = newArr3(double,nxn*XLEN,nyn*YLEN,nzn*ZLEN);
	double*** TXY = newArr3(double,nxn*XLEN,nyn*YLEN,nzn*ZLEN);
	double*** TXZ = newArr3(double,nxn*XLEN,nyn*YLEN,nzn*ZLEN);
	double*** TYY = newArr3(double,nxn*XLEN,nyn*YLEN,nzn*ZLEN);
	double*** TYZ = newArr3(double,nxn*XLEN,nyn*YLEN,nzn*ZLEN);
	double*** TZZ = newArr3(double,nxn*XLEN,nyn*YLEN,nzn*ZLEN);
	double*** TPAR  = newArr3(double,nxn*XLEN,nyn*YLEN,nzn*ZLEN);
	double*** TPER1 = newArr3(double,nxn*XLEN,nyn*YLEN,nzn*ZLEN);
	double*** TPER2 = newArr3(double,nxn*XLEN,nyn*YLEN,nzn*ZLEN);
	double*** EPS = newArr3(double,nxn*XLEN,nyn*YLEN,nzn*ZLEN);
	double*** agyro_scudder = newArr3(double,nxn*XLEN,nyn*YLEN,nzn*ZLEN);
	double*** agyro_aunai = newArr3(double,nxn*XLEN,nyn*YLEN,nzn*ZLEN);
	double*** nongyro_swisdak = newArr3(double,nxn*XLEN,nyn*YLEN,nzn*ZLEN);
	double*** align = newArr3(double,nxn*XLEN,nyn*YLEN,nzn*ZLEN);
	double*** smoothed = newArr3(double,nxn*XLEN,nyn*YLEN,nzn*ZLEN);

	string temp;

		temp = "recflux.txt";
		ofstream my_file(temp.c_str());

	for (int it = initlevel+rank; it < nlevels+1; it=it+size){
	     printf( "Process %d doing level %d\n", rank, it );

	//Electric field

    readvect(it, "/fields/","E", EX, EY, EZ);
    writeVTKvect_binary(it, "E", "", EX, EY, EZ);

	//Magnetic field
    readvect(it,"/fields/","B", BX, BY, BZ);
    writeVTKvect_binary(it,"B", "", BX, BY, BZ);


    double recflux = 0.0;
    recflux = vecpot(AZ, BX, BY, BZ);
    writeVTKscalar_binary(it, "Az", "", AZ);
    cout << recflux << endl;
    my_file << it * DeltaT << "   " << recflux << endl;

	//Compute ExB
    //cross(BX, BY, BZ, EX, EY, EZ, VXBX, VXBY, VXBZ)

	//Rho by species
    readscalar(it,"/moments/species_0/","rho",  NE);
    readscalar(it,"/moments/species_1/","rho",  NI);
    if (ns >4) readscalar(it,"/moments/species_4/","rho",  NO);
    //writeVTKscalar_species_binary(it,"rhoHarris", NE, NI, NO);

    if (ns >3) addreadscalar(it,"/moments/species_2/","rho",  NE);
    if (ns >2) addreadscalar(it,"/moments/species_3/","rho",  NI);
    if (ns >5) addreadscalar(it,"/moments/species_5/","rho",  NO);

    if (ns >4) writeVTKscalar_species_binary(it,"rho", NE, NI, NO);
    if (ns <5) writeVTKscalar_species_binary(it,"rho", NE, NI);
   // writeVTKscalar("rho", "e", VX);

	//Currents species0
    readvect(it,"/moments/species_0/","J",  VX, VY, VZ);
 //   writeVTKvect(it,"JHarris", "e", VX, VY, VZ);
    if (ns >2) addreadvect(it,"/moments/species_2/","J",  VX, VY, VZ);
    writeVTKvect_binary(it,"J", "e", VX, VY, VZ);


    //Pressure tensor species 0
    readscalar(it,"/moments/species_0/","pXX",  TXX);
    readscalar(it,"/moments/species_0/","pXY",  TXY);
    readscalar(it,"/moments/species_0/","pXZ",  TXZ);
    readscalar(it,"/moments/species_0/","pYY",  TYY);
    readscalar(it,"/moments/species_0/","pYZ",  TYZ);
    readscalar(it,"/moments/species_0/","pZZ",  TZZ);
    
    if (ns >2) addreadscalar(it,"/moments/species_2/","pXX",  TXX);
    if (ns >2) addreadscalar(it,"/moments/species_2/","pXY",  TXY);
    if (ns >2) addreadscalar(it,"/moments/species_2/","pXZ",  TXZ);
    if (ns >2) addreadscalar(it,"/moments/species_2/","pYY",  TYY);
    if (ns >2) addreadscalar(it,"/moments/species_2/","pYZ",  TYZ);
    if (ns >2) addreadscalar(it,"/moments/species_2/","pZZ",  TZZ);

    extract_pressure(qom[0], BX, BY, BZ, VX, VY, VZ, NE,
          TXX, TXY, TXZ, TYY, TYZ, TZZ, TPAR, TPER1, TPER2, EPS);
    writeVTKtensor_binary(it, "P", "e", TXX, TXY, TXZ, TYY, TYZ, TZZ,
    	  TPAR, TPER1, TPER2, EPS);

    agyro(agyro_scudder, agyro_aunai, nongyro_swisdak, align,
    	BX, BY, BZ, TXX, TXY, TXZ, TYY, TYZ, TZZ);
    writeVTKscalar_binary(it, "agyro_scudder", "", agyro_scudder);
    writeVTKscalar_binary(it, "agyro_aunai", "", agyro_aunai);
    int Nsmooth = 3;
    smooth(Nsmooth, nongyro_swisdak, smoothed, nxn*XLEN, nyn*YLEN, nzn*ZLEN);
    writeVTKscalar_binary(it, "nongyro_swisdak", "", smoothed);

    smooth(Nsmooth, align, smoothed, nxn*XLEN, nyn*YLEN, nzn*ZLEN);
    writeVTKscalar_binary(it, "align", "", smoothed);


    //Currents species1
    readvect(it,"/moments/species_1/","J",  VX, VY, VZ);
 //   writeVTKvect(it,"JHarris", "i", VX, VY, VZ);
    if (ns >2) addreadvect(it,"/moments/species_3/","J",  VX, VY, VZ);
    writeVTKvect_binary(it,"J", "i", VX, VY, VZ);


    //Pressure tensor species 1
       readscalar(it,"/moments/species_1/","pXX",  TXX);
       readscalar(it,"/moments/species_1/","pXY",  TXY);
       readscalar(it,"/moments/species_1/","pXZ",  TXZ);
       readscalar(it,"/moments/species_1/","pYY",  TYY);
       readscalar(it,"/moments/species_1/","pYZ",  TYZ);
       readscalar(it,"/moments/species_1/","pZZ",  TZZ);

       if (ns >2) addreadscalar(it,"/moments/species_3/","pXX",  TXX);
       if (ns >2) addreadscalar(it,"/moments/species_3/","pXY",  TXY);
       if (ns >2) addreadscalar(it,"/moments/species_3/","pXZ",  TXZ);
       if (ns >2) addreadscalar(it,"/moments/species_3/","pYY",  TYY);
       if (ns >2) addreadscalar(it,"/moments/species_3/","pYZ",  TYZ);
       if (ns >2) addreadscalar(it,"/moments/species_3/","pZZ",  TZZ);

       extract_pressure(qom[1], BX, BY, BZ, VX, VY, VZ, NI,
             TXX, TXY, TXZ, TYY, TYZ, TZZ, TPAR, TPER1, TPER2, EPS);
       writeVTKtensor_binary(it, "P", "i", TXX, TXY, TXZ, TYY, TYZ, TZZ,
       	  TPAR, TPER1, TPER2, EPS);



    if (ns >4){
    //Currents species2
    readvect(it,"/moments/species_4/","J",  VX, VY, VZ);
    if (ns >5) addreadvect(it,"/moments/species_5/","J",  VX, VY, VZ);
    writeVTKvect_binary(it,"J", "o", VX, VY, VZ);


    }
}

	my_file.close();

	delArr3(BX,nxn*XLEN,nyn*YLEN);
	delArr3(BY,nxn*XLEN,nyn*YLEN);
	delArr3(BZ,nxn*XLEN,nyn*YLEN);
	delArr3(EX,nxn*XLEN,nyn*YLEN);
	delArr3(EY,nxn*XLEN,nyn*YLEN);
	delArr3(EZ,nxn*XLEN,nyn*YLEN);
	delArr3(VX,nxn*XLEN,nyn*YLEN);
	delArr3(VY,nxn*XLEN,nyn*YLEN);
	delArr3(VZ,nxn*XLEN,nyn*YLEN);
	delArr3(AZ,nxn*XLEN,nyn*YLEN);
	delArr3(agyro_scudder,nxn*XLEN,nyn*YLEN);
	delArr3(agyro_aunai,nxn*XLEN,nyn*YLEN);
	delArr3(nongyro_swisdak,nxn*XLEN,nyn*YLEN);
	delArr3(align,nxn*XLEN,nyn*YLEN);
	delArr3(smoothed,nxn*XLEN,nyn*YLEN);

	delete[] temp_storageX;
	delete[] temp_storageY;
	delete[] temp_storageZ;
	
	
    MPI_Finalize();
    return(0);
}

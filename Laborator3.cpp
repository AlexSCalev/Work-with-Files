#include <iostream>
#include<stdio.h>
#include<stdlib.h>
#include <time.h>
#include "mpi.h"
using namespace std;

void show2DMatrix(int **matrix,int numbRows,int numbCols) {
    for(int i = 0; i < numbRows; i++ ) {
        for(int j = 0; j < numbCols; j++) {
            cout << " [ " << i << " " << j << " ] " << matrix[i][j] << " ";
        }
        cout << endl;
    }
}
// Found Max Value
int getMaxValue(int *array , int numbElements) {
    int max = array[0];
    for(int i = 0; i < numbElements; i++) {
        if(array[i] > max) {
            max = array[i];
        }
    }
    return max;
}
int main( int argc, char *argv[] )
{   
    MPI_Init(&argc,&argv);
    int rank , size_ranks;
    MPI_Comm_rank(MPI_COMM_WORLD , &rank);
    
    int numbRows,numbCols;
    int **matrix;

    if(rank == 0) {
        // Generate user matrix
        cout << "Set number of rows : ";
        cin >> numbRows;
        cout << endl;
        cout << "Set number of Cols : ";
        cin >> numbCols;
        cout << endl;
        // Generate memory for matrix and fill matrix
        matrix = new int *[numbRows];
        for(int i = 0; i < numbRows; i++) {
            matrix[i] = new int [numbCols];
        }
        // Fill the matrix 
        // int count = 0;
        srand(time(NULL));
        for(int i = 0; i < numbRows; i++) {
            for(int j = 0; j < numbCols; j++) {
                matrix[i][j] = 1+rand()%100;
                // count++;
            }
        }
        // Send to write number rows 
        MPI_Send(&numbRows, 1 , MPI_INT , 0 , 0 , MPI_COMM_WORLD);
        // Send to write number cols 
        MPI_Send(&numbCols, 1 , MPI_INT , 0 , 0 , MPI_COMM_WORLD);
        // Send Generate matrix to write 
        MPI_Send( &**matrix , numbCols*numbRows , MPI_INT , 0 , 0 , MPI_COMM_WORLD);
    }
    MPI_Barrier(MPI_COMM_WORLD);
    
    
    // Create group write and read
    MPI_Group write_group,read_group;
    MPI_Comm_group(MPI_COMM_WORLD, &write_group);
    MPI_Comm_group(MPI_COMM_WORLD, &read_group);
 
    // Create communcator write and read
    MPI_Comm write_comm,read_comm;
    MPI_Comm_create(MPI_COMM_WORLD, write_group, &write_comm);
    MPI_Comm_create(MPI_COMM_WORLD, read_group, &read_comm);
   
    // Get write rank from write communicator
    int write_rank,size_write_group;
    MPI_Comm_rank( write_comm , &write_rank);
    MPI_Comm_size( write_comm , &size_write_group);
    
    // set file and name the file where will save our dates
    MPI_File myfile;
    char filename[] = "writeFile.dat";
// Get matrix and send to all 
    if(write_rank == 0) {
        // Read to number rows 
        MPI_Recv(&numbRows , 1 , MPI_INT , 0 , 0 , MPI_COMM_WORLD , MPI_STATUS_IGNORE);
        // Read to number cols 
        MPI_Recv(&numbCols , 1 , MPI_INT , 0 , 0 , MPI_COMM_WORLD , MPI_STATUS_IGNORE);
        // Read Generate matrix to write 
        MPI_Recv(&**matrix  , numbRows*numbCols , MPI_INT , 0 , 0 , MPI_COMM_WORLD , MPI_STATUS_IGNORE);
        cout << " Original Matrix " << endl;

        show2DMatrix(matrix,numbRows,numbCols);

        // Create File if it dosen`t exsit
        
        MPI_File_open(MPI_COMM_SELF,filename,MPI_MODE_CREATE | MPI_MODE_WRONLY,MPI_INFO_NULL, &myfile);
		int n = 0;
        MPI_File_write(myfile, &n, 1, MPI_INT,MPI_STATUS_IGNORE);
		MPI_File_close(&myfile);
        
        // devide dates to all procs
        int numbElements = numbRows*numbCols;
        while (true)
        { 
            if(numbElements%size_write_group == 0) {
                break;   
            }
            numbElements++;
        }

        int diplacament = numbElements/size_write_group;

            int *send_buffer = new int [numbElements];
            int count_buffer = 0;
            for(int i = 0; i < numbRows; i++) {
                for(int j = 0; j < numbCols; j++) {
                    send_buffer[count_buffer] = matrix[i][j];
                    count_buffer++;
                }
            }
            for(int i = count_buffer; i < numbElements; i++ ) {
                send_buffer[i] = 0;
            }

            // Send dates to all procs write procs
            int *send_buf = new int[diplacament];
            int buf_count = 0;
            int state = 1;
            int count_rank = 0;
             for(int i = 0; i < numbElements; i++ , state++ ) {
                 send_buf[buf_count] = send_buffer[i];
                 if(state%diplacament == 0) {
                    MPI_Send(&*send_buf , diplacament , MPI_INT , count_rank  , 0 , write_comm);
                    buf_count = 0;
                    count_rank++;
                 } else {
                     buf_count++;
                 }  
            }
    }


        
        MPI_Barrier(write_comm);
        // Get send dates
        MPI_Status status;
        MPI_Probe( 0 , 0, write_comm, &status);
        
        // Get the number of send elements
        int count;
        MPI_Get_count(&status, MPI_INT, &count);
        int *buf_recv = new int [count];
        MPI_Recv(&*buf_recv , count , MPI_INT , 0 , 0 , write_comm , MPI_STATUS_IGNORE);
        // Get Communicator name
        char processor_name[MPI_MAX_PROCESSOR_NAME];
        int namelen;
        MPI_Get_processor_name(processor_name,&namelen);
        cout << " This rank " << write_rank << " from communicator " << processor_name  << " Send next Elements : " << endl;
        for(int i = 0; i < count; i++) {
            cout << " " << buf_recv[i] << endl;
        }
        // write in file
    MPI_File_open(MPI_COMM_SELF,filename, MPI_MODE_APPEND|MPI_MODE_WRONLY ,MPI_INFO_NULL, &myfile);
	MPI_File_write(myfile, &*buf_recv, count, MPI_INT,MPI_STATUS_IGNORE);
	MPI_File_close(&myfile);
    MPI_Barrier(write_comm);



    // Read zone
    int read_rank , size_read_ranks;
    MPI_Comm_rank(read_comm , &read_rank);
    MPI_Comm_size(read_comm , &size_read_ranks);
    if(read_rank == 0) {
        cout << "-----------------------------------" << endl;     

        int numbElements = numbRows*numbCols;
        while (true)
        { 
            if(numbElements%size_write_group == 0) {
                break;   
            }
            numbElements++;
        }
        int *read_buff = new int[numbElements];
        MPI_File_open(MPI_COMM_SELF,filename, MPI_MODE_RDONLY |MPI_MODE_DELETE_ON_CLOSE ,MPI_INFO_NULL, &myfile);
        MPI_File_read( myfile , &*read_buff , numbElements , MPI_INT , MPI_STATUS_IGNORE);
	    MPI_File_close(&myfile);

            // for(int i = 0; i < numbElements; i++) {
            //     cout << read_buff[i] << endl;
            // }

            // cout << " ---------------------------------------------- " << endl;
            // Send date to all procs
    
            int diplacament = numbElements/size_write_group;

            int *send_buf = new int[diplacament];
            int buf_count = 0;
            int state = 1;

            int count_rank = 0;
             for(int i = 0; i < numbElements; i++ , state++ ) {
                 send_buf[buf_count] = read_buff[i];
                 if(state%diplacament == 0) {
                    MPI_Send(&*send_buf , diplacament , MPI_INT , count_rank  , 0 , read_comm);
                    buf_count = 0;
                    count_rank++;
                 } else {
                     buf_count++;
                 }  
            }       
    }

    MPI_Barrier(write_comm);
        
        MPI_Status status1;
        MPI_Probe( 0 , 0, read_comm, &status1);
        
        // // Get the number of send elements
        int count1;
        MPI_Get_count(&status1, MPI_INT, &count1);
        int *buf_recv1 = new int [count1];
        MPI_Recv(&*buf_recv1 , count1 , MPI_INT , 0 , 0 , read_comm , MPI_STATUS_IGNORE);
        // // Foind max elements
        // for(int i = 0; i < count1; i++) {
        //     cout << " " << buf_recv1[i] << endl;
        // }
        int max = getMaxValue(buf_recv1,count1);
        int maximize = 0;
        MPI_Reduce(&max, &maximize, 1, MPI_INT, MPI_MAX, 0, read_comm);
        MPI_Barrier(read_comm);
        if(read_rank == 0) {
            cout << "Max elements from matrix is : " << maximize << endl;
        }
   
        MPI_Barrier(read_comm);
      
  
    MPI_Finalize();
    return 0;
}
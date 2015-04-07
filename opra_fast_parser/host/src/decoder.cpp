// Copyright (C) 2013-2014 Altera Corporation, San Jose, California, USA. All rights reserved.
// Permission is hereby granted, free of charge, to any person obtaining a copy of this
// software and associated documentation files (the "Software"), to deal in the Software
// without restriction, including without limitation the rights to use, copy, modify, merge,
// publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to
// whom the Software is furnished to do so, subject to the following conditions:
// The above copyright notice and this permission notice shall be included in all copies or
// substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
// OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
// HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
// WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
// OTHER DEALINGS IN THE SOFTWARE.
// 
// This agreement shall be governed in all respects by the laws of the State of California and
// by the laws of the United States of America.

// ACL specific includes
#include "CL/opencl.h"
#include "AOCLUtils/aocl_utils.h"
#include "file_parse.h"
#include "msg_io.h"
#include "aocl_net.h"


#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sched.h>
#include <stdlib.h>


#define DISPLAY 1000
#define REF_FIELDS 5


static const short fpga_port = 65000;
static const char *fpga_ip = "192.168.7.4";
static const short host_port = 65000;
static const char *host_ip = "192.168.7.43";


using namespace aocl_utils;

typedef unsigned char uchar;

static cl_platform_id platform = NULL;
static cl_device_id device = NULL;
static cl_context context = NULL;
static cl_command_queue queue0 = NULL, queue1 = NULL, queue2 = NULL, queue3 = NULL, queue4 = NULL, queue5 = NULL;
static cl_kernel trading_kernel = NULL, decoder_kernel = NULL;
static cl_kernel width_adapt_2to1_kernel = NULL, width_adapt_1to2_kernel = NULL;
static cl_kernel io_in_kernel = NULL, io_out_kernel = NULL;
static cl_program program = NULL;
static cl_int status = 0;

void error(const char *str) {
   printf("%s", str);
   exit(0);
}

bool init() {
  cl_int status;

  // Get the OpenCL platform.
  platform = findPlatform("Altera");
  if(platform == NULL) {
    printf("ERROR: Unable to find Altera OpenCL platform\n");
    return false;
  }

  // Query the available OpenCL devices.
  scoped_array<cl_device_id> devices;
  cl_uint num_devices;

  devices.reset(getDevices(platform, CL_DEVICE_TYPE_ALL, &num_devices));

  // We'll just use the first device.
  device = devices[0];

  // Create the context.
  context = clCreateContext(NULL, 1, &device, &oclContextCallback, NULL, &status);
  checkError(status, "Failed to create context");

  // Create the command queues.
  queue0 = clCreateCommandQueue(context, device, 0, &status);
  checkError(status, "Failed to create command queue");
  queue1 = clCreateCommandQueue(context, device, 0, &status);
  checkError(status, "Failed to create command queue");
  queue2 = clCreateCommandQueue(context, device, 0, &status);
  checkError(status, "Failed to create command queue");
  queue3 = clCreateCommandQueue(context, device, 0, &status);
  checkError(status, "Failed to create command queue");
  queue4 = clCreateCommandQueue(context, device, 0, &status);
  checkError(status, "Failed to create command queue");
  queue5 = clCreateCommandQueue(context, device, 0, &status);
  checkError(status, "Failed to create command queue");  
  // Create the program.
  std::string binary_file = getBoardBinaryFile("opra_fast_parser", device);
  printf("Using AOCX: %s\n\n", binary_file.c_str());
  program = createProgramFromBinary(context, binary_file.c_str(), &device, 1);
  // Build the program that was just created.
  status = clBuildProgram(program, 0, NULL, "", NULL, NULL);
  checkError(status, "Failed to build program");

  // Create the kernel - name passed in here must match kernel name in the
  // original CL file, that was compiled into an AOCX file using the AOC tool
  decoder_kernel = clCreateKernel(program, "OPRADecoder", &status);
  checkError(status, "Failed to create kernel");
  trading_kernel = clCreateKernel(program, "Trading", &status);
  checkError(status, "Failed to create kernel");

  width_adapt_1to2_kernel = clCreateKernel(program, "width_adapt_1to2", &status);
  checkError(status, "Failed to create kernel");
  width_adapt_2to1_kernel = clCreateKernel(program, "width_adapt_2to1", &status);
  checkError(status, "Failed to create kernel");
  status = clEnqueueTask(queue3, width_adapt_1to2_kernel, 0, NULL, NULL);
  checkError(status, "Failed to launch kernel");
  status = clEnqueueTask(queue2, width_adapt_2to1_kernel, 0, NULL, NULL);
  checkError(status, "Failed to launch kernel");

  io_in_kernel = clCreateKernel(program, "io_in_kernel", &status);
  checkError(status, "Failed to create kernel");
  io_out_kernel = clCreateKernel(program, "io_out_kernel", &status);
  checkError(status, "Failed to create kernel");

  return true;
}

// Host and device buffers
ulong *h_inData;
ulong *h_outData, *h_verifyData;
int *frameSizes;
uchar *h_rawData;
cl_mem d_inData, d_outData;
int frameCount = 1000000; // input frameSizes

#define TIME_OUT 10000

// Free the resources allocated during initialization
void cleanup() {
  if(trading_kernel) 
    clReleaseKernel(trading_kernel);  
  if(decoder_kernel) 
    clReleaseKernel(decoder_kernel);  
  if(width_adapt_1to2_kernel) 
    clReleaseKernel(width_adapt_1to2_kernel);  
  if(width_adapt_2to1_kernel) 
    clReleaseKernel(width_adapt_2to1_kernel);  
  if(io_out_kernel) 
    clReleaseKernel(io_out_kernel);      
  if(io_in_kernel) 
    clReleaseKernel(io_in_kernel);      
  if(program) 
    clReleaseProgram(program);
  if(queue0) 
    clReleaseCommandQueue(queue0);
  if(queue1) 
    clReleaseCommandQueue(queue1);
  if(queue2) 
    clReleaseCommandQueue(queue2);
  if(queue3) 
    clReleaseCommandQueue(queue3);
  if(queue4) 
    clReleaseCommandQueue(queue4);
  if(queue5) 
    clReleaseCommandQueue(queue5);    
  if(context) 
    clReleaseContext(context);
  if(h_inData)
	alignedFree(h_inData);
  if (h_outData)
	alignedFree(h_outData);
  if (d_inData)
	clReleaseMemObject(d_inData);
  if (d_outData) 
	clReleaseMemObject(d_outData);
}

const char *file_name = "opra_data.pcap";
const char *file_format = "pcap";
extern FILE* l_cur_pcap_file;
FILE *output;
      
int sock;
struct sockaddr_in serveraddr, clientaddr;

int pipeEnds[2];

double run_opra(int frameCount, uchar start_field, uchar end_field, bool useUDPInput, bool useUDPOutput, bool sendOnly, bool rcvOnly, bool reference, bool compact) {
  uchar *dataPtr = h_rawData;
  int crtFrame = 0;
  int messages = 0;
  int uniqueSize = 0;
  int uniqueMessages = 0;
  
  // Initialize packet data from the input file
  init_input_read(file_name, file_format);
  while (crtFrame < frameCount) {
    int len;
    while (l_cur_pcap_file == NULL || (len = get_pkt(l_cur_pcap_file, dataPtr, MAX_FRAME_SIZE)) == 0) {
       if (uniqueSize == 0) uniqueSize = dataPtr - h_rawData;
       if (uniqueSize == 0) uniqueSize = dataPtr - h_rawData;
       if (uniqueMessages == 0) uniqueMessages = messages;
       init_input_read(file_name, file_format);
    }
    frameSizes[crtFrame] = len;
    for (int i = OPRA_HEADER_SIZE; i < len - 1;) {
       messages++;
       i += dataPtr[i] + 1;
    }
    dataPtr += len;
    crtFrame++;
  }
  if (uniqueSize == 0) uniqueSize = dataPtr - h_rawData;
  if (uniqueMessages == 0) uniqueMessages = messages;

  // Compact small UDP frameSizes into larger ones; this is required to achieve high bandwidth
  if (compact) {
    int to = 0, from = 0;
    int largeFrame = -1;
    for (int f = 0; f < frameCount; f++) {
       if (from == uniqueSize) {
          uniqueSize = to + 1;
       }
       int len = frameSizes[f];
       if ((uniqueSize > to + 1 || to % uniqueSize  != uniqueSize - 1) && largeFrame >= 0 && len + frameSizes[largeFrame] - OPRA_HEADER_SIZE < MAX_FRAME_SIZE) {
           frameSizes[largeFrame] += len - OPRA_HEADER_SIZE - 1;
           from += OPRA_HEADER_SIZE;
           for (int i = OPRA_HEADER_SIZE; i < len - 1; i++) h_rawData[to++] = h_rawData[from++];
           from++;
       } else {
           if (largeFrame >= 0) {
              frameSizes[largeFrame]++; 
              h_rawData[to++] = 3;  // OPRA EOP
           }
           frameSizes[++largeFrame] = len - 1;
           for (int i = 0; i < len - 1; i++) h_rawData[to++] = h_rawData[from++];
           from++;
       }
    }
    h_rawData[to++] = 3; // OPRA EOP
    frameSizes[largeFrame]++;
    frameCount = largeFrame + 1;
    if (to + 1 < uniqueSize) uniqueSize = to + 1;
  }

  // Create h_inData for memory based access
  // 16 bytes of data, 16 bytes of control
  int pos = 0, decoder_size = 0, input_size = 0;
  
  uchar* h_inData_char = (uchar*)h_inData;
  
  
  for (int f = 0; f < frameCount; f++) {
    int data_left = frameSizes[f];
    int count = 0;
    
    decoder_size   +=  (frameSizes[f] + 8 - 1) / 8;

    while(data_left > 0){
      int copy_size = data_left < 16 ?  data_left :16;
      memcpy(h_inData_char, &h_rawData[pos],copy_size);
      
      h_inData_char += 16;
      pos += copy_size;
      data_left -= copy_size;

      memset(h_inData_char,0,16);
      
      if(count==0){
        h_inData_char[0] |= (1L << 0);;
      } 
      if (data_left == 0) {     
        h_inData_char[0]  |= (ulong)(1L << 1);
        h_inData_char[0] = (h_inData_char[0] & ~(0x0fL << 2)) | ((16-copy_size) << 2);    
      } 
      h_inData_char += 16;
      input_size+=2;
    }
  }
  
  if (useUDPInput) {
    // Do not copy data
    // Instead, send it after the kernel starts
  } else {
    // Copy data from host to device
    status = clEnqueueWriteBuffer(queue4, d_inData, CL_TRUE, 0, 2 * sizeof(ulong) * input_size, h_inData, 0, NULL, NULL);
    checkError(status, "Failed to copy data to device");
    clFinish(queue4);
  }
  
  // Prepare the kernels for execution
  uchar count = end_field - start_field + 1;

  // Set the OPRA Decoder flags
  uchar read_from = useUDPInput ? 0x01 : 0x00;
  
  if (!rcvOnly) {

    status = clSetKernelArg(decoder_kernel, 0, sizeof(cl_int), (void*)&decoder_size);
    checkError(status, "Failed to set kernel arg 0");
    
    // Set the dummy Trading kernel arguments
    int udp_packet_length = MAX_FRAME_SIZE / sizeof(ulong); // arbitrary size for UDP packets
    uchar write_to = useUDPOutput ? 0x01 : 0x00;
    int out_size = messages * count;

    status = clSetKernelArg(trading_kernel, 0, sizeof(cl_uchar), (void*)&start_field);
    checkError(status, "Failed to set kernel arg 0");
    status = clSetKernelArg(trading_kernel, 1, sizeof(cl_uchar), (void*)&count);
    checkError(status, "Failed to set kernel arg 1");
    status = clSetKernelArg(trading_kernel, 2, sizeof(cl_int), (void*)&udp_packet_length);
    checkError(status, "Failed to set kernel arg 2");
    status = clSetKernelArg(trading_kernel, 3, sizeof(cl_int), (void*)&out_size);
    checkError(status, "Failed to set kernel arg 3");

    status = clEnqueueTask(queue1, trading_kernel, 0, NULL, NULL);
    checkError(status, "Failed to launch kernel");

    // Set the input kernel to read from channel or memory
    status = clSetKernelArg(io_in_kernel, 0, sizeof(cl_mem), (void *)&d_inData);
    checkError(status, "Failed to set kernel arg 0");
    status = clSetKernelArg(io_in_kernel, 1, sizeof(cl_uchar), (void*)&read_from);
    checkError(status, "Failed to set kernel arg 1");
    status = clSetKernelArg(io_in_kernel, 2, sizeof(cl_int), (void*)&input_size);
    checkError(status, "Failed to set kernel arg 2");
 
    status = clEnqueueTask(queue4, io_in_kernel, 0, NULL, NULL);
    checkError(status, "Failed to launch kernel");
    
    // Set the output kernel to read from channel or memory
    status = clSetKernelArg(io_out_kernel, 0, sizeof(cl_mem), (void *)&d_outData);
    checkError(status, "Failed to set kernel arg 0");
    status = clSetKernelArg(io_out_kernel, 1, sizeof(cl_uchar), (void*)&write_to);
    checkError(status, "Failed to set kernel arg 1");
    status = clSetKernelArg(io_out_kernel, 2, sizeof(cl_int), (void*)&out_size);
    checkError(status, "Failed to set kernel arg 2");

    status = clEnqueueTask(queue5, io_out_kernel, 0, NULL, NULL);
    checkError(status, "Failed to launch kernel");
  
  }

  // Clear receive buffer
  for (int i = 0; i < 2 * messages * count; i++) { h_outData[i] = 0; }
  
  // Drain any packets that are already waiting
  int total = 0;
  int pkts = 0;
  while (int j = recvfrom(sock, h_outData, MAX_FRAME_SIZE, MSG_DONTWAIT, NULL, NULL) > 0) {   
    total += j;
    pkts++;
  };
  if ( pkts > 0 ) {
     printf("Unexpected packets received: Drained %d packets, %d bytes in total\n",pkts,total); fflush(stdout);
  }
  
  // Measure the execution time
  struct timeval start, end;

  if (!useUDPInput) {
     // Get the iteration stamp to evaluate performance
     gettimeofday(&start, NULL);
  }

  if (!rcvOnly) {
     status = clEnqueueTask(queue0, decoder_kernel, 0, NULL, NULL);
     checkError(status, "Failed to launch kernel");
  }
  
  if (useUDPInput) {
     gettimeofday(&start, NULL);
  }

  // Main loop in UDP communications
  // Interleaves send and receive
  // Uses a reduced buffer size to localize data and keep it in cache (required
  // to achieve higher throughput consistently)

  int sent = 0, frame = 0, received = 0;
  int totalReq = 0;
  int timeToRecv = 0;
  bool startedReceiving = false;
  int timeOut = 0;
  bool ok = true;
  int pkts_sent=0;
  int pkts_rcvd = 0;
  bool moreWork;

  do {
     timeToRecv++;
     moreWork = false;
     if (useUDPInput && frame < frameCount && !rcvOnly) {
        // Write packets to UDP socket
        int max = 0;
        int ret;
        do {
           ret = sendto(sock, h_rawData + sent % uniqueSize, frameSizes[frame], MSG_DONTWAIT, (const sockaddr*)&clientaddr, sizeof(clientaddr));
           if (ret > 0) {
              frame++;
              sent += ret;
              totalReq += ret;
           }
           max++;
        } while (max < 64 && ret > 0 && frame < frameCount);
        moreWork = frame < frameCount;
        if (!moreWork) {clFinish(queue0);  clFinish(queue4); }  // Required to prevent interference from signal 44
     }
     
     if (useUDPOutput && !sendOnly) {
        // Read packets from UDP socket
        int msg_len = MAX_FRAME_SIZE;
        int ret;
           ulong buf[MAX_FRAME_SIZE / 8];
           do {
              ret = recvfrom(sock, buf, msg_len, MSG_DONTWAIT, NULL, NULL);
              for (int i = 0; i < ret; i += 8) {
                 if (!rcvOnly || received + i < uniqueMessages * 8) {
                    *((ulong *)h_outData + (received + i) / 8) = buf[i / 8];
                 } else {
                    if (*((ulong *)h_outData + ((received + i) / 8) % (uniqueMessages)) != buf[i / 8]) {
                       if (ok) {
                          *((ulong *)h_outData + ((received + i) / 8) % (uniqueMessages)) = buf[i / 8]; // Copy incorrect data to propagate error
                          ok = false;
                       }
                    }
                 }
              }
              // The host PC may drop packets, so eventually we can time out after waiting for all packets
              if (ret > 0) {
                 startedReceiving = true;
                 timeOut = 0;
                 if (received == 0) gettimeofday(&start, NULL);
                 received += ret;
              } else {
                 if (startedReceiving) timeOut++;
                 if (timeOut == TIME_OUT) {
                   printf(" Receiver timed out after receiving %d out of %d bytes\n",received,
                       messages * count * sizeof(ulong));
                    for (int i = received/sizeof(ulong); i < uniqueMessages; i++) *((ulong *)h_outData + i) = 0xBEEF;
                 }
              }
           } while (ret > 0);
        if (received < messages * count * sizeof(ulong) && timeOut < TIME_OUT) {
           moreWork = true;
        } else {
           if (moreWork) {
              error("Error: Finished receiving before transmitting\n");
           }
           gettimeofday(&end, NULL);
           if (!rcvOnly) { clFinish(queue1);  clFinish(queue5);}
        }
     } else {
        if (!moreWork) {
           clFinish(queue1); 
           clFinish(queue5);
           gettimeofday(&end, NULL);
        }
     }
  
  } while (moreWork);
  
 long seconds  = end.tv_sec  - start.tv_sec;
 long useconds = end.tv_usec - start.tv_usec;
 double time = seconds + useconds/1000000.0;

  if (useUDPInput && !useUDPOutput) {
     clFinish(queue0);    
     clFinish(queue4);
  }

  // Wait for kernels to finish
  if (useUDPOutput && !useUDPInput) {
     // Data is already on the host
     clFinish(queue1);
     clFinish(queue5);
  } 
  
  // Read back data
  if (!useUDPOutput) {
     // Copy results from device to host
     status = clEnqueueReadBuffer(queue5, d_outData, CL_TRUE, 0, 2 * sizeof(ulong) * count * messages, h_outData, 0, NULL, NULL);
     checkError(status, "Failed to copy data from device");
     clFinish(queue5); 
     while (recvfrom(sock, h_outData, MAX_FRAME_SIZE, MSG_DONTWAIT, NULL, NULL) > 0) {
        printf("Error: unexpected data\n");
        break;
     };

  }
  
  // Verify data received is correct

  //if we are using memory,
  if (!(rcvOnly && useUDPOutput)) {
     if (sendOnly && useUDPInput) {
        int left = uniqueMessages * count * sizeof(ulong);
        while (left) {
           left -= read(pipeEnds[0], h_outData + uniqueMessages * count * sizeof(ulong) - left, left);
        }
        for (int i = uniqueMessages; i < messages; i += uniqueMessages) {
           for (int j = 0; j < uniqueMessages * count; j++) {
              h_outData[i * count + j] = h_outData[j];
           }
        }
     }
     if (reference) {
        for (int i = 0; i < messages; i++) {
           for (int j = start_field; j <= end_field; j++) {
              h_verifyData[count * i + j] = h_outData[(i * count + j)];
           }
        }
     } else {
        int errors = 0;
        for (int i = 0; i < messages; i++) {
           for (int j = start_field; j <= end_field; j++) {
              if (h_verifyData[i * REF_FIELDS + j] != h_outData[(i * count + j - start_field)]) {
                 if ( errors < 50 )
                    printf("Error %d decoding message %d at field %d, %lx != %lx\n", errors,i, j, 
                          h_verifyData[i * REF_FIELDS + j], 

                          h_outData[(i * count + j - start_field)]);
                 errors++;
                 break;
              }
           }
        }
        if (errors == 0)
          printf("        Verified %d messages for fields %d-%d\n",messages,start_field,end_field);
        else
        {
          printf("        Found %d errors in %d messages for fields %d-%d\n",errors,messages,start_field,end_field);
          return -1;
        }
     }
  } else {
     write(pipeEnds[1], h_outData, uniqueMessages * count * sizeof(ulong));
     if (timeOut == TIME_OUT) return -1;
  }
    
  // Return throughput in Gbps
  return input_size * sizeof(ulong) * 8 / time / 1e9;
}

#define FRAME_BUF 1000000000

int main (int argc, char *argv[]) {
   // Options.
   Options options(argc, argv);

   bool useUDPInput = true;
   bool useUDPOutput = true;

   if(options.has("in")) {
      std::string inputSrc = options.get<std::string>("in");
      if(inputSrc == "udp") {
         useUDPInput = true;
      }
      else if(inputSrc == "mem") {
         useUDPInput = false;
      }
      else {
         printf("Error: -in must be one of 'udp' or 'mem'.\n");
         return 1;
      }

   }
   if(options.has("out")) {
      std::string inputSrc = options.get<std::string>("in");
      if(inputSrc == "udp") {
         useUDPOutput = true;
      }
      else if(inputSrc == "mem") {
         useUDPOutput = false;
      }
      else {
         printf("Error: -out must be one of 'udp' or 'mem'.\n");
         return 1;
      }
   }

   if(options.has("framecount")) {
      frameCount = options.get<unsigned>("framecount");
   }

   bool sendOnly = false, rcvOnly = false;

   if (!setCwdToExeDir()) {
      return false;
   }

   // Set CPU affinity and fork process when doing UDP transfers on both send
   // and receive paths

   cpu_set_t set;
   CPU_ZERO(&set);
   CPU_SET(3, &set);
   if (sched_setaffinity(0, sizeof(cpu_set_t), &set) != 0) error("Error setting affinity mask\n");
   if (useUDPInput && useUDPOutput) {
      if (pipe(pipeEnds) == -1) error("Error: can't create pipe");
      int pid = fork();
      if (pid == 0) {
         close(pipeEnds[1]);
         sendOnly = true;
         CPU_ZERO(&set);
         CPU_SET(1, &set);
         if (sched_setaffinity(0, 1, &set) != 0) error("Error: setting affinity mask\n");
      } else {
         close(pipeEnds[0]);
         rcvOnly = true;
      }
   }
   
   if (!rcvOnly) {   
      if(!init()) {
        return false;
      } else if (useUDPInput || useUDPOutput) {
        // Initialization will force reprogram, wait for transceivers to come back up before using quickudp
        sleep(5);
        int ret = aocl_quickudp_init( "acl0", 0, "55:55:aa:aa:11:11", fpga_ip, "255.255.255.0");
        if ( ret == 0 )
          ret = aocl_quickudp_open_session( "acl0", 0, 0, fpga_port, host_ip, fpga_port, host_port);
        if ( ret != 0 )
        {
          printf("Failed to open UDP connection\n");
          return false;
        }
      }
   }
   
   // Allocate frame buffer
   h_rawData = (uchar *)alignedMalloc(FRAME_BUF);
   frameSizes = (int *)malloc(sizeof(int) * frameCount);
   // Allocate host memory
   h_inData = (ulong *)alignedMalloc(FRAME_BUF);
   h_outData = (ulong *)alignedMalloc(FRAME_BUF);
   h_verifyData = (ulong *)alignedMalloc(FRAME_BUF);

   if (!rcvOnly) {
      d_inData = clCreateBuffer(context, CL_MEM_READ_WRITE, FRAME_BUF, NULL, &status);
      checkError(status, "Failed to allocate input device buffer\n");
      d_outData = clCreateBuffer(context, CL_MEM_READ_WRITE, FRAME_BUF, NULL, &status);
      checkError(status, "Failed to allocate output device buffer\n");
   }

   if (useUDPInput || useUDPOutput) {
      sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
      if (sock < 0) {
         error("Error: Failed opening socket\n");
      }

      // Open the socket for two way communication
      memset(&serveraddr, 0, sizeof(serveraddr));
      serveraddr.sin_family = AF_INET;
      serveraddr.sin_port = htons(host_port);
      if (!sendOnly) {
         if (bind(sock, (struct sockaddr *)&serveraddr, sizeof(serveraddr)) < 0) {
            error("Error: bind failed\n");
         }
      }
      clientaddr.sin_family = AF_INET;
      clientaddr.sin_port = htons(fpga_port);
      if (inet_aton(fpga_ip, &clientaddr.sin_addr) == 0) {
         error("Error: Wrong IP address\n");
      }
   }

   // 1st execution - do memory-based verification
   if (!rcvOnly) {
      int frames = frameCount < DISPLAY ? frameCount : DISPLAY;
      printf("Long run, use as reference for subsequent runs\n");
      run_opra(frameCount, 0, REF_FIELDS - 1, false, false, sendOnly, rcvOnly, true, false);
      
      output = fopen( "results.txt","w+" );
      dump_decoded_msg( output, h_verifyData, frames, FILE_DUMP );
      fclose(output);
       
      printf("Use compact UDP packets for better throughput\n");

      run_opra(frameCount, 0, REF_FIELDS - 1, false, false, sendOnly, rcvOnly, false, false);
      output = fopen( "results2.txt","w+" );
      dump_decoded_msg( output, h_outData, frames, FILE_DUMP );
      fclose(output);

      printf("All integrity checks are DONE.\n\n");
      printf("Performance testing: %s --> OPRA decoder --> %s\n", useUDPInput ? "UDP Rx" : "memory", useUDPOutput ? "UDP Tx" : "memory");
   }
   // 2nd execution - use requested media
   double maxThr = 0;

   // Run for all fields that are valid in all message types
   for (int i = 0; i < REF_FIELDS; i++) {
      double thr;
      int retry = -1;
      do {
         thr = run_opra(frameCount, i, i, useUDPInput, useUDPOutput, sendOnly, rcvOnly, false, true);
         retry++;
         if (!rcvOnly) 
            printf("        Throughput = %g for field %d on retry %d from %s\n",thr,i,retry, rcvOnly ? "Receiver" : "Sender");
      } while (thr < 0);
      if (thr > maxThr) maxThr = thr;
   }
   
   // Print maximum throughput, cleanup resources and exit
   if (!rcvOnly) printf("\n\tOPRA stream throughput: %f Gbit / s\n", maxThr);
   printf("%s thread terminated\n",rcvOnly ? "Receiver" : "Sender");

   if (useUDPInput || useUDPOutput) {
      shutdown(sock, 2);
      close(sock);
   }

   cleanup();

   return 0;
}


#include<stdio.h>
#include<stdlib.h>
#include<string.h>

#define WORK_VERSION 210

typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned int uint32;

typedef struct{
  uint8 Version;
  uint8 VideoWidth;
  uint8 VideoHeight;
  uint8 VideoDepth;
  uint16 FrameCount;
  uint8 Compress;
  uint32 VideoRecordId;
  char Description[30];
  uint16 AudioRate;
  uint8 FrameRate;
} MovieType;


uint8 version;
uint8 width;
uint8 height;
uint8 depth;
uint16 frame_count;
uint8 compress;
uint32 video_record_id;
char *description;
uint16 audio_rate;
uint8 frame_rate;


struct MFrameType{
  uint16 BitmapSize;
  uint8* BitmapBits;
  uint16 AudioSize;
  uint8* AudioData;
};


/*

sub getRGB {
    my @rgb = @_;
    my $tempR = $rgb[0];
    my $tempB = $rgb[1];

    my $R=$rgb[0] >> 3;
    my $B=$rgb[1] & 0x1f;

    my $GL=($tempR & 0x07) << 2; 
    my $GR=$tempB >> 6;
    my $G=$GL | $GR;
    print OUTFILE "$R $G $B";
    #print "R=$R G=$G B=$B\n";
}

*/


int main(int argc,
	 char * argv[]){
  
  FILE *infile;
  FILE *fcount;

  MovieType *input_movie;
  char read_buffer[1024];
  int result;

  //fprintf(stdout, "Size of uchar %u ushort %u uint %u\n",
  //  sizeof(unsigned char), sizeof(unsigned short), sizeof(unsigned int));
  
  infile = fopen(argv[1],"r");
  if(infile == NULL){
    fprintf(stderr, "Error opening input file\n");
    exit(1);
  }
  
  result = fread(read_buffer, 1, 46, infile);
  if(result == 0){
    fprintf(stderr, "Reached end of file soon\n");
    exit(1);
  }

  input_movie = (MovieType *)read_buffer;

  //check the version of the file
  if(input_movie->Version != WORK_VERSION){
    fprintf(stderr, "You are not using the right version of MovieRec file\n with the movierec2mpeg converter\n");
    exit(1);
  }

  //fprintf(stdout, "Read %d bytes\n", result);
  fprintf(stdout, "Version: %d\n", input_movie->Version);
  fprintf(stdout, "Video Width: %d\n", input_movie->VideoWidth);
  fprintf(stdout, "Video Height: %d\n", input_movie->VideoHeight);
  fprintf(stdout, "Video Depth: %d\n", input_movie->VideoDepth);
  fprintf(stdout, "Compress: %d\n", input_movie->Compress);
  fprintf(stdout, "Frame Count(Audio+Video): %d\n", ntohs(input_movie->FrameCount));
  fprintf(stdout, "Video Record ID: %d\n", ntohl(input_movie->VideoRecordId));
  fprintf(stdout, "Description: %s\n", input_movie->Description);
  fprintf(stdout, "Audio Rate: %d\n", ntohs(input_movie->AudioRate)); 
  fprintf(stdout, "Frame Rate: %d\n", input_movie->FrameRate); 

  version =  input_movie->Version;
  width =  input_movie->VideoWidth;
  height =  input_movie->VideoHeight;
  depth =  input_movie->VideoDepth;
  compress =  input_movie->Compress;
  frame_count =  ntohs(input_movie->FrameCount)/2;
  video_record_id =  ntohl(input_movie->VideoRecordId);
  description =  strdup(input_movie->Description);
  audio_rate =  ntohs(input_movie->AudioRate); 
  frame_rate =  input_movie->FrameRate; 

  
  //write the frame count to framecount.dat file
  fcount = fopen("framecount.dat","w");
  if(fcount == NULL){
    fprintf(stderr, "Error opening framecount.dat file\n");
    exit(1);
  }
  
  result = fprintf(fcount, "%d",frame_count);
  if(result == 0){
    fprintf(stderr, "Cannot write to framecount.dat file\n");
    exit(1);
  }
  fclose(fcount);
  
  int i = 0;

  for(i = 1; i < frame_count; ++i){//change this to read lastframe

    FILE *ppmfile_ptr;

    fprintf(stdout, "#################################################\n");
    fprintf(stdout, "    FRAME # %d\n", i);

    uint16 *temp;

    uint16 bitmap_size = 0;
    uint8 *bitmap;
    uint16 audio_size = 0;
    uint8 *audio_data;

    result = fread(read_buffer, 1, sizeof(uint16), infile);
    if(result == 0){
      fprintf(stderr, "Reached end of file before reading bitmap size\n");
      exit(1);
    }
    temp = (uint16 *)read_buffer;
    bitmap_size = ntohs(*temp);
    fprintf(stdout, "Bitmap Size: %u.. ", bitmap_size);
    
    bitmap = (uint8 *)malloc(bitmap_size);
    
    //read the bitmap
    result = fread(bitmap, 1, bitmap_size, infile);
    if(result == 0){
      fprintf(stderr, "Could not read bitmap\n");
      exit(1);
    }
    fprintf(stdout, "Read %d bytes of bitmap\n", result);
    
    //make the ppm
    char *ppm_file;
    int m = 0;
    int wrap_count = 0;
    ppm_file = (char *)malloc(20);
    
    sprintf(ppm_file,"Image_%d.ppm",i);
    ppmfile_ptr = fopen(ppm_file, "w");
    if(ppmfile_ptr == NULL){
      fprintf(stdout,"Error: Unable to create ppm file\n");
      free(bitmap);
      free(audio_data);
      fclose(infile);
      
      exit(1);
    }

    fprintf(ppmfile_ptr, "P3\n");
    fprintf(ppmfile_ptr, "%d %d\n", width, height);
    fprintf(ppmfile_ptr, "63\n");

        
    for(m=0; m<bitmap_size; m+=2){
      //fprintf(stdout, "RGB = %u %u -> ",bitmap[m],bitmap[m+1]);
      uint8 tempR = bitmap[m];
      uint8 tempB = bitmap[m+1];

      uint8 R = bitmap[m] >> 3;
      uint8 B = bitmap[m+1] & 0x1f;

      uint8 GL = (tempR & 0x07) << 2;
      uint8 GR = tempB >> 6;
      
      uint8 G = GL | GR;
      fprintf(ppmfile_ptr, "%u %u %u",R,G,B);
      
      wrap_count = wrap_count+6;
      if(wrap_count >= 66){
	fprintf(ppmfile_ptr, "\n");
	wrap_count = 0;
      }
      else{
	fprintf(ppmfile_ptr, " ");
      }
    }
    fclose(ppmfile_ptr);
    free(ppm_file);

    result = fread(read_buffer, 1, sizeof(uint16), infile);
    if(result == 0){
      fprintf(stderr, "Reached end of file before reading audio data size\n");
      exit(1);
    }
    temp = (uint16 *)read_buffer;
    audio_size = ntohs(*temp);
    fprintf(stdout, "Audio Size: %u..", audio_size);
    
    //read audio data
    audio_data = (uint8 *)malloc(audio_size);
    result = fread(audio_data, 1, audio_size, infile);
    if(result == 0){
      fprintf(stderr, "Could not read audio data\n");
      exit(1);
    }
    fprintf(stdout, "Read %d bytes of audio data\n", result);
    
    //write audio data to file
    {
      FILE *audio_file;
      char *afname;
      
      afname = (char *)malloc(20);
      sprintf(afname,"Image_%d.pcm",i);
      
      audio_file = fopen(afname, "w");
      if(audio_file == NULL){
	fprintf(stdout,"Error writing audion file\n");
	free(bitmap);
	free(audio_data);
	fclose(infile);
	exit(1);
      }
      
      result = fwrite(audio_data, 1, audio_size, audio_file);
      if(result == 0){
	fprintf(stderr, "Could not write audio data to file\n");
	exit(1);
      }
      fclose(audio_file);
      free(afname);
    }

    free(bitmap);
    free(audio_data);
    
  }
  fclose(infile);

  fprintf(stdout, "*************************************************\n");
  fprintf(stdout, "Create mpeg.dat.. ");
  {
    FILE *mpeg_dat;
    char *outmpeg;
    
    outmpeg = (char *)malloc(strlen(argv[1])+20);
    sprintf(outmpeg,"%s.mpg",argv[1]);
    
    mpeg_dat = fopen("mpeg.dat","w");
    if(mpeg_dat == NULL){
      fprintf(stdout, "Error: Opening mpeg.dat file\n");
      exit(1);
    }
    
    fprintf(mpeg_dat, "PATTERN IBBPBBPBBPBBPBB\n");
    fprintf(mpeg_dat, "OUTPUT %s\n",outmpeg);
    fprintf(mpeg_dat, "INPUT_DIR .\n");
    fprintf(mpeg_dat, "INPUT\n");
    fprintf(mpeg_dat, "Image_*.ppm [1-%d]\n",frame_count-1);
    //change this to read last frame
    fprintf(mpeg_dat, "END_INPUT\n");
    fprintf(mpeg_dat, "BASE_FILE_FORMAT PPM\n");
    fprintf(mpeg_dat, "INPUT_CONVERT *\n");
    fprintf(mpeg_dat, "GOP_SIZE 1\n");
    fprintf(mpeg_dat, "SLICES_PER_FRAME 160\n");
    fprintf(mpeg_dat, "PIXEL FULL\n");
    fprintf(mpeg_dat, "PQSCALE 8\n");
    fprintf(mpeg_dat, "IQSCALE 8\n");
    fprintf(mpeg_dat, "BQSCALE 8\n");
    fprintf(mpeg_dat, "RANGE 4\n");
    fprintf(mpeg_dat, "PSEARCH_ALG LOGARITHMIC\n");
    fprintf(mpeg_dat, "BSEARCH_ALG SIMPLE\n");
    fprintf(mpeg_dat, "REFERENCE_FRAME DECODED\n");
    fprintf(mpeg_dat, "FRAME_RATE 25\n");
    fprintf(mpeg_dat, "FORCE_ENCODE_LAST_FRAME\n");
    fclose(mpeg_dat);
  }
  fprintf(stdout, "done\n");
  return 0;
}

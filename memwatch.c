/*
 *
 *
 * Usage ./AppMemory Appname
 */



#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/sysinfo.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/unistd.h>
#include <dirent.h>
#include <libgen.h>
#include <unistd.h>

int main(int ac, char *av[]);
int CheckUsage(const char*);


/*Function open the statm file for the Application and monitor the usage constantly*/
int CheckUsage(const char *filename)
{

  struct sysinfo info;

  if( sysinfo(&info)!= 0 )
    perror("Sysinfo\n");

  unsigned long TotalRam = 0;
  TotalRam = (info.totalram * info.mem_unit) / 1024;

  printf("%10s %12s %13s","Total Ram","Application","Percentage\n");
  //printf("Check file %s\n", filename);


  while (1) {

      FILE *FP;
      FP = fopen(filename,"r");

      if(FP == NULL){
          fprintf(stderr,"Application close or File '%s' does not exists\n", filename);
          return 1;
      }

      unsigned long size=0;
      unsigned long resident=0;
      unsigned long share=0;
      unsigned long text=0;
      unsigned long lib=0;
      unsigned long data=0;
      unsigned long dt=0;

      fscanf(FP,"%lu %lu %lu %lu %lu %lu %lu",&size, &resident, &share, &text, &lib, &data, &dt);

      unsigned long TotalUsage = (resident * getpagesize());

      float Percentage = 0.0f;
      Percentage = (float)(((float)TotalUsage/1024)/(float)TotalRam)*100;


      printf("%10lukB %10lukB %10.3f%%\n", TotalRam, TotalUsage/1024, Percentage);

      fclose(FP);

      sleep(1);
  }

  return 1;
}

int main(int ac, char *av[])
{
  const char *AppName = av[1];
  if(AppName==NULL){
      fprintf(stderr,"Usage %s Appname\n", (char *)basename(av[0]));
      exit(EXIT_FAILURE);
  }

  DIR *ProcDir = NULL;

  DIR *ProcDirDir = NULL;
  struct dirent *pDir = NULL;
  struct dirent *pDirDir = NULL;

  ProcDir = opendir("/proc");
  if (ProcDir == NULL){
      fprintf(stderr,"Can't Open /proc directory \n");
      return 1;
  }


  char buf[128] = "";
  while ((pDir = readdir(ProcDir)) != NULL){

      /*skip . and ..*/
      if(strlen(pDir->d_name)<=2)continue;

      /*skip file and symlink directory*/
      if(((int)pDir->d_type == 8) || ((int)pDir->d_type == 10)) continue;
      //printf("Directory is %s %d\n", pDir->d_name, (int)pDir->d_type);

      sprintf(buf,"/proc/%s", pDir->d_name);

      /*Open Directory inside the proc directory*/
      ProcDirDir = opendir(buf);
      while ((pDirDir = readdir(ProcDirDir))!=NULL){

          if (strlen(pDirDir->d_name)<=2 )continue;

          if((pDirDir->d_type == 4) || (pDirDir->d_type == 10))continue;

          //printf("Inside Directory %s \n", pDirDir->d_name)

          if(strcmp(pDirDir->d_name,"cmdline") == 0){

              char cmdline[128] = "";
              sprintf(cmdline,"/proc/%s/%s",pDir->d_name, pDirDir->d_name);
              //printf("We found the cmdline file for %s of %s %s\n", pDirDir->d_name, pDir->d_name, cmdline);

              FILE *fp;
              fp = fopen(cmdline,"r");
              if(fp==NULL){
                  fprintf(stderr,"Can't open file %s \n", cmdline);
                  closedir(ProcDirDir);
                  closedir(ProcDir);
                  exit(EXIT_FAILURE);

              }

              /*We found the Application under /proc/pid by parsing cmdline file, cmdline has filepath as relative,
               * using basename get the Application name only
               */

              char *data = malloc(512 * sizeof(char));
              fgets(data,512,fp);
              const char *cmd = (const char *)basename (data);
              free(data);

              if(strcmp(cmd,AppName)==0){

                  //printf("Found Command %s %d\n", cmd, strlen(cmd));
                  sprintf(cmdline,"/proc/%s/statm",pDir->d_name);

                  CheckUsage(cmdline);
              }

              fclose(fp);
          }


      } //while loop to parse directory inside proc directory
      closedir(ProcDirDir);

  } //while parse proc dir

  closedir(ProcDir);


  return EXIT_SUCCESS;
}

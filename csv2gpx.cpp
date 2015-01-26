/*
 * copyright (c) 2015 Sam C. Lin
 * convert GPS Master CSV file to GPX with HR extension
 *
 * 20150126 SCL v0.1 first rev
 *
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <limits.h>

#define VERSTR "Lincomatic GPS Master CSV to GPX Converter v0.1\n\n"

#define MAX_PATH 256
#define MAX_PTS 32768

class Datum {
  char *buf;
public:
  char *time;
  char *satcnt;
  char *hr;
  char *speed;
  char *lon;
  char *lat;
  char *alt;
  char *heading;
  char *dist;
  Datum() { buf = NULL; }
  ~Datum() { if (buf) delete [] buf; }
  int Set(char *line);
};

int Datum::Set(char *line)
{
  int slen = strlen(line);
  buf = new char[slen+1];
  if (buf) {
    strcpy(buf,line);
    buf[slen] = 0;

    char *s = buf;
    s = strchr(s,',');
    *(s++) = 0;
    this->time = s;
    *(strchr(this->time,' ')) = 'T';
    s = strchr(s,',');
    *(s++) = 0;
    this->satcnt = s;
    s = strchr(s,',');
    *(s++) = 0;
    this->hr = s;
    s = strchr(s,',');
    *(s++) = 0;
    this->speed = s;
    s = strchr(s,',');
    *(s++) = 0;
    this->lon = s;
    s = strchr(s,',');
    *(s++) = 0;
    this->lat = s;
    s = strchr(s,',');
    *(s++) = 0;
    this->alt = s;
    s = strchr(s,',');
    *(s++) = 0;
    this->heading = s;
    s = strchr(s,',');
    *(s++) = 0;
    this->dist = s;
    
    return 0;
  }

  return 1;
}


Datum pts[MAX_PTS];
int ptCnt;
char line[4096];
// read line and convert from UTF-16
int readLine(FILE *fp)
{
  int i=0;
  for (;;) {
    char c0,c1;
    if ((fread(&c0,1,1,fp) == 1) && 
	(fread(&c1,1,1,fp) == 1)) {
      if (c1 != 0) {
	return 2;
      }
      if (c0 == 0x0a) {
	line[i++] = '\0';
	return 0;
      }
      if (c0 == '\t') {
	line[i++] = ',';
      }
	  else if ((c0 != 0x0d) && (c0 != '"')) {
	line[i++] = c0;
      }

    }
    else {
      return 1;
    }
  }
}


int readCSV(char *fn)
{
  FILE *fp = fopen(fn,"rb");
  if (fp) {
    // throw away header FFFE
    if (fread(line,2,1,fp) != 1) {
      return 1;
    }
	readLine(fp); // throw away header line
    while (!readLine(fp)) {
      if (pts[ptCnt++].Set(line)) {
	printf("Out of Memory! point %d\n",ptCnt);
	return 2;
      }
    }
    fclose(fp);
	return 0;
  }
  return 1;
}

double ft2m(char *sft)
{
  double ft;
  sscanf(sft,"%lf",&ft);
  return ft * .30480;
}

void makeTrackPoint(Datum *dt)
{
  char bpm[200];

  if (*dt->hr == '0') {
    bpm[0] = 0;
  }
  else {
    sprintf(bpm,"<extensions><gpxtpx:TrackPointExtension><gpxtpx:hr>%s</gpxtpx:hr></gpxtpx:TrackPointExtension></extensions>",dt->hr);
  }


  sprintf(line,"<trkpt lat=\"%s\" lon=\"%s\"><ele>%lf</ele><time>%sZ</time>%s</trkpt>\n",dt->lat,dt->lon,ft2m(dt->alt),dt->time,bpm);
}
int writeGPX(char *sport,char *fn)
{
  FILE *fp = fopen(fn,"wb");
  if (fp) {
    int i;
    double avgHR=0.0,maxHR=0.0;
    int ptcnt = 0;
    for (i=0;i < ptCnt;i++) {
      double hr = strtod(pts[i].hr,NULL);
      if (hr) {
	if (hr > maxHR) {
	  maxHR = hr;
	}
	avgHR += hr;
	ptcnt++;
      }
    }

    int iavgHR = (int)(ceil(avgHR /= (double)ptcnt));
    int imaxHR = (int) maxHR;

    printf("Avg HR: %d\n",iavgHR);
    printf("Max HR: %d\n",imaxHR);
    printf("Trackpoints %d\n", ptCnt);

    // write header
    fprintf(fp,"<?xml version=\"1.0\" encoding=\"UTF-8\"?><gpx  version=\"1.1\"  creator=\"RunKeeper - http://www.runkeeper.com\"  xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"  xmlns=\"http://www.topografix.com/GPX/1/1\"  xsi:schemaLocation=\"http://www.topografix.com/GPX/1/1 http://www.topografix.com/GPX/1/1/gpx.xsd\"  xmlns:gpxtpx=\"http://www.garmin.com/xmlschemas/TrackPointExtension/v1\"><trk>\n");
    fprintf(fp,"  <name><![CDATA[%s %sZ]]></name>\n",sport,pts[0].time);
    fprintf(fp,"  <time>%sZ</time>\n",pts[0].time);
    fprintf(fp,"<trkseg>\n");
    for (i=0;i < ptCnt;i++) {
      makeTrackPoint(&pts[i]);
      // write track point
      if (fprintf(fp,line) != strlen(line)) {
	return 2;
      }
    }

    // write footer
    fprintf(fp,"</trkseg>\n</trk>\n</gpx>\n");


    fclose(fp);
    return 0;
  }
  return 1;
}

int main(int argc,char *argv[])
{
  printf(VERSTR);
  if (argc != 2) {
    printf("Usage: csv2gpx infile\n");
    return 1;
  }

  char outfn[MAX_PATH];
  int slen = strlen(argv[1]);
  strcpy(outfn,argv[1]);
  if (outfn[slen-4] == '.') {
    strcpy(outfn + slen -3,"gpx");
  }
  else {
    strcat(outfn,".gpx");
  }
  printf("Converting %s -> %s\n",argv[1],outfn);

  ptCnt = 0;
  if (!readCSV(argv[1])) {
    writeGPX("Other",outfn);
  }

  return 0;
}

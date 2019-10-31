
#!/usr/bin/perl

# June 3, 2005 - mvr2mpg initial version 1.0
#
# Copyright © 2005 Thomas A. Fine
# License is herby granted to distribute and use freely, for commercial and
# non-commercial reasons, in whole or in part, provided that this copyright
# notice and license are maintained.
#
# Required programs: pnmcat, pnmhisteq, ffmpeg
# provide full paths, if needed:

$PNMCAT="pnmcat";
$PNMHISTEQ="pnmhisteq";
$FFMPEG="ffmpeg";

$outfile="out.mpg";

$fix_contrast=0;
if ($#ARGV >= 0) {
  if ($ARGV[0] eq "-c") {
    shift(@ARGV);
    $fix_contrast=1;
  }
}

if ($#ARGV == -1) {
  &usage;
}

$infile=shift(@ARGV);

if ($#ARGV == 0) {
  $outfile=shift(@ARGV);
  if ($outfile !~ /.mpg$/i) {
    $outfile .= ".mpg";
  }
} elsif ($#ARGV > 0) {
  &usage;
}

open(IN,$infile);

read(IN,$buf,1);  $version=ord($buf);
read(IN,$buf,1);  $width=ord($buf);
read(IN,$buf,1);  $height=ord($buf);
read(IN,$buf,1);  $depth=ord($buf);
read(IN,$buf,2);  $framecount=unpack("n",$buf);
read(IN,$buf,1);  $compress=ord($buf);
read(IN,$buf,1);  $eat=ord($buf);
read(IN,$buf,4);  $id=unpack("N",$buf);
read(IN,$buf,30); $description=$buf;
read(IN,$buf,2);  $audiorate=unpack("n",$buf);
read(IN,$buf,1);  $framerate=ord($buf);
read(IN,$buf,1);  $eat=ord($buf);

print "version=$version\n";
print "width=$width\n";
print "height=$height\n";
print "depth=$depth\n";
print "framecount=$framecount\n";
print "compress=$compress\n";
print "id=$id\n";
print "audiorate=$audiorate\n";
print "framerate=$framerate\n";
print "description=$description\n";

open(AUD,">/tmp/mvraud.out") || die "Can't open audio out!\n";

for ($i=0; $i<$framecount; ++$i) {
  read(IN,$buf,2);  $bitmapsize=unpack("n",$buf);
  last if (eof(IN));
  print "reading $bitmapsize bitmap\n";
  if (($nr=read(IN,$buf,$bitmapsize)) != $bitmapsize) {
    print "$nr != $bitmapsize\n";
    exit(1);
  }
  $frame=sprintf("/tmp/mvr.%d.ppm",$i);

  if ($fix_contrast) {
    open(PPM,"|$PNMHISTEQ >$frame") || die "Can't open frame $i out\n";
  } else {
    # pnmcat is used to convert the ascii PPM that I produce into raw
    # PPM required by ffmpeg
    open(PPM,"|$PNMCAT -lr >$frame") || die "Can't open frame $i out\n";
  }
  print PPM "P3\n$width $height\n255\n";
  for ($j=0; $j<$bitmapsize; $j+=2) {
    $x1=ord(substr($buf,$j,1));
    $x2=ord(substr($buf,$j+1,1));
    $r=$x1>>3;
    $b=$x2 & 0x1f;
    $g=(($x1&0x07)<<2) | ($x2>>6);
    $r *= 4; $g *= 4; $b *= 4;
    print PPM "$r $g $b ";
  }
  print PPM "\n";
  close(PPM);

  read(IN,$buf,2);  $audiosize=unpack("n",$buf);
  last if (eof(IN));
  print "reading $audiosize audio\n";
  if (($nr=read(IN,$buf,$audiosize)) != $audiosize) {
    print "$nr != $audiosize\n";
    exit(1);
  }
  print AUD $buf;
}
close(AUD);
print "$i frames read\n";

if (!eof(IN)) {
  print "error! Not EOF!\n";
}

#the data header from movierec output says 32000 samples per second and
#15 frames per second.  That doesn't quite jibe - if it's 15 frames per
#second then I'm only acutally seeing 30720 samples per second.
#But it's so close, and the moveis so short, I'll ignore that for now.

system("$FFMPEG -ar 32000 -f s16be -i /tmp/mvraud.out -r 15 -i /tmp/mvr.%d.ppm -r 30 $outfile");

for ($i=0; $i<$framecount; ++$i) {
  $frame=sprintf("/tmp/mvr.%d.ppm",$i);
  unlink($frame);
}
unlink("/tmp/mvraud.out");

#
# END OF MAIN
#

sub usage {
  print "usage: $0 [-c] mvr_file [outfile]\n";
  print "       -c automatically adjusts contrast for each frame\n";
  print "       \".mpg\" will be appended to outfile, if not included\n";
  print "       If outfile is not given, \"out.mpg\" is used.\n";
  exit(0);
}

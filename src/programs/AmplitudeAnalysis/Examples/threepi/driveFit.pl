#!/usr/bin/perl

$nBins = 65;
$fitName = "threepi_0pol_fit";

$workingDir = "/Users/mashephe/amptools/examples/threepi_example";
$bin = "/Users/mashephe/iu_cvs/GlueXAmpExe";

$fitDir = "$workingDir/$fitName";
$lastParams = "$workingDir/$fitName/par_seed.txt";

for( $i = 0; $i < $nBins; ++$i ){

  chdir $fitDir;
  chdir "bin_$i";

  open( PARINIT, ">param_init.cfg" ) or die;
  if( !open( LAST, $lastParams ) ){

    print "WARNING:  Can't find parameters from the fit in bin ".($i-1)."\n";
    print "          This likely means the fit failed in that bin.  Using\n";
    print "          parameters from last good fit as a seed for this bin.\n";
    print "          (Press <RETURN> to continue.)\n";
    $input = <STDIN>;

    $lastParams = $lastSuccessfulParams;
  }

  $lastSuccessfulParams = $lastParams;

  $nLine = 1;
  $nAmp = 0;
  while( <LAST> ){

    if( $nLine == 1 ){ 

      if( /(\S+)\s+(\S+)/ ){

	$nAmp = $1;
	$nPar = $2;
	
      }
    }
    if( ( $nLine > 1 ) && ( $nLine <= $nAmp + 1 ) ){

      if( /(\S+)\s+\(([-\d\.]+)\s*\,\s*([-\d.]+)\)/ ){
	
	$amp = $1;
	$re  = $2;
	$im  = $3;

	if( $amp =~ /(\S+)\+$/ ){  $amp = $1; }

	if( $nLine == 2 ){

	  print PARINIT "initialize $amp cartesian $re $im fixedphase\n";
	}
	else{

	  print PARINIT "initialize $amp cartesian $re $im\n";
	}
      }
    }
    if( ( $nLine > $nAmp + 1 ) && ( $nLine <= $nAmp + $nPar + 1 ) ){

      if( /(\S+)\s+(\S+)/ ){

	print PARINIT "parameter $1 $2";
      }
    }
    $nLine++;
  }
  close PARINIT;

  print  "$bin/fit -c bin_$i.cfg\n";
  system( "$bin/fit -c bin_$i.cfg" );

  $lastParams = "$fitDir/bin_$i/fit.bin_$i.txt";

  chdir $fitDir;
}


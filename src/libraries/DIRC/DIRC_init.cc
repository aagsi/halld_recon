/*
 * DIRC_init.cc
 *
 *  Created on: Oct 11, 2012
 *      Author: yqiang
 *  Modified on: 0ct 7, 2013, yqiang, added DIRCTruthHit factory
 */

#include <JANA/JEventLoop.h>
using namespace jana;

#include "DDIRCTDCDigiHit.h"
#include "DDIRCPmtHit.h"
#include "DDIRCLEDRef.h"
#include "DDIRCTruthBarHit.h"
#include "DDIRCTruthPmtHit.h"
#include "DDIRCLut_factory.h"
#include "DDIRCPmtHit_factory.h"
#include "DDIRCLEDRef_factory.h"
#include "DDIRCGeometry_factory.h"

jerror_t DIRC_init(JEventLoop *loop) {

	/// Create and register DIRC data factories
	loop->AddFactory(new JFactory<DDIRCTDCDigiHit>());
	loop->AddFactory(new DDIRCGeometry_factory());
	loop->AddFactory(new DDIRCPmtHit_factory());
	loop->AddFactory(new DDIRCLEDRef_factory());
	loop->AddFactory(new JFactory<DDIRCTruthPmtHit>());
	loop->AddFactory(new JFactory<DDIRCTruthBarHit>());
	loop->AddFactory(new DDIRCLut_factory());

	return NOERROR;
}


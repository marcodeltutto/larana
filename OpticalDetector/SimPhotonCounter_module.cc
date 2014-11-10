// \file SimPhotonCounter.h 
// \author Ben Jones, MIT 2010
//
// Module to determine how many phots have been detected at each OpDet
//
// This analyzer takes the SimPhotonsCollection generated by LArG4's sensitive detectors
// and fills up to four trees in the histograms file.  The four trees are:
//
// OpDetEvents       - count how many phots hit the OpDet face / were detected across all OpDet's per event
// OpDets            - count how many phots hit the OpDet face / were detected in each OpDet individually for each event
// AllPhotons      - wavelength information for each phot hitting the OpDet face
// DetectedPhotons - wavelength information for each phot detected
//
// The user may supply a quantum efficiency and sensitive wavelength range for the OpDet's.
// with a QE < 1 and a finite wavelength range, a "detected" phot is one which is
// in the relevant wavelength range and passes the random sampling condition imposed by
// the quantum efficiency of the OpDet
//
// PARAMETERS REQUIRED:
// int32   Verbosity          - whether to write to screen a well as to file. levels 0 to 3 specify different levels of detail to display
// string  InputModule        - the module which produced the SimPhotonsCollection
// bool    MakeAllPhotonsTree - whether to build and store each tree (performance can be enhanced by switching off those not required)
// bool    MakeDetectedPhotonsTree
// bool    MakeOpDetsTree
// bool    MakeOpDetEventsTree
// double  QantumEfficiency   - Quantum efficiency of OpDet
// double  WavelengthCutLow   - Sensitive wavelength range of OpDet
// double  WavelengthCutHigh
#ifndef SimPhotonCounter_h
#define SimPhotonCounter_h 1

// ROOT includes.
#include "TTree.h"
#include "TFile.h"
#include <Rtypes.h>

// FMWK includes
#include "art/Framework/Core/EDAnalyzer.h"
#include "art/Framework/Core/ModuleMacros.h"
#include "art/Framework/Principal/Event.h"
#include "fhiclcpp/ParameterSet.h"
#include "art/Framework/Principal/Handle.h"
#include "art/Persistency/Common/Ptr.h"
#include "art/Persistency/Common/PtrVector.h"
#include "art/Framework/Services/Registry/ServiceHandle.h"
#include "art/Framework/Services/Optional/TFileService.h"
#include "art/Framework/Services/Optional/TFileDirectory.h"
#include "messagefacility/MessageLogger/MessageLogger.h"

// LArSoft includes
#include "PhotonPropagation/PhotonVisibilityService.h"
#include "Simulation/SimListUtils.h"
#include "Simulation/sim.h"
#include "Simulation/LArG4Parameters.h"

// ROOT includes
#include <TH1D.h>
#include <TF1.h>
#include <TTree.h>

// C++ language includes
#include <iostream>
#include <sstream>
#include <cstring>
#include <vector>

#include "CLHEP/Random/RandFlat.h"
#include "CLHEP/Random/RandGaussQ.h"

namespace opdet {

  class SimPhotonCounter : public art::EDAnalyzer{
    public:
      
      SimPhotonCounter(const fhicl::ParameterSet&);
      virtual ~SimPhotonCounter();
      
      void analyze(art::Event const&);
      
      void beginJob();
      void endJob();
     
    private:
      
      // Trees to output

      TTree * fThePhotonTreeAll;
      TTree * fThePhotonTreeDetected;
      TTree * fTheOpDetTree;
      TTree * fTheEventTree;


      // Parameters to read in

      std::string fInputModule;      // Input tag for OpDet collection

      int fVerbosity;                // Level of output to write to std::out

      bool fMakeDetectedPhotonsTree; //
      bool fMakeAllPhotonsTree;      //
      bool fMakeOpDetsTree;         // Switches to turn on or off each output
      bool fMakeOpDetEventsTree;          //
      
      float fQE;                     // Quantum efficiency of tube

      float fWavelengthCutLow;       // Sensitive wavelength range 
      float fWavelengthCutHigh;      // 


      

      // Data to store in trees

      Float_t fWavelength;
      Float_t fTime;
      Int_t fCount;
      Int_t fCountOpDetAll;
      Int_t fCountOpDetDetected;

      Int_t fCountEventAll;
      Int_t fCountEventDetected;
      
      Int_t fEventID;
      Int_t fOpChannel;


    };
}

#endif


namespace opdet {
  

  SimPhotonCounter::SimPhotonCounter(fhicl::ParameterSet const& pset)
    : EDAnalyzer(pset)
  {
    fVerbosity=                pset.get<int>("Verbosity");
    fInputModule=              pset.get<std::string>("InputModule");
    fMakeAllPhotonsTree=       pset.get<bool>("MakeAllPhotonsTree");
    fMakeDetectedPhotonsTree=  pset.get<bool>("MakeDetectedPhotonsTree");
    fMakeOpDetsTree=           pset.get<bool>("MakeOpDetsTree");
    fMakeOpDetEventsTree=      pset.get<bool>("MakeOpDetEventsTree");
    fQE=                       pset.get<double>("QuantumEfficiency");
    fWavelengthCutLow=         pset.get<double>("WavelengthCutLow");
    fWavelengthCutHigh=        pset.get<double>("WavelengthCutHigh");
    // get the random number seed, use a random default if not specified    
    // in the configuration file.  
    unsigned int seed = pset.get< unsigned int >("Seed", sim::GetRandomNumberSeed());
    createEngine(seed);    
  }
  

  void SimPhotonCounter::beginJob()
  {
    // Get file service to store trees
    art::ServiceHandle<art::TFileService> tfs;

     // Create and assign branch addresses to required tree
    if(fMakeAllPhotonsTree)
      {
	fThePhotonTreeAll = tfs->make<TTree>("AllPhotons","AllPhotons");
	fThePhotonTreeAll->Branch("EventID",     &fEventID,          "EventID/I");
	fThePhotonTreeAll->Branch("Wavelength",  &fWavelength,       "Wavelength/F");
	fThePhotonTreeAll->Branch("OpChannel",       &fOpChannel,            "OpChannel/I");
	fThePhotonTreeAll->Branch("Time",        &fTime,             "Time/F");
      }

    if(fMakeDetectedPhotonsTree)
      {
	fThePhotonTreeDetected = tfs->make<TTree>("DetectedPhotons","DetectedPhotons");
	fThePhotonTreeDetected->Branch("EventID",     &fEventID,          "EventID/I");
	fThePhotonTreeDetected->Branch("Wavelength",  &fWavelength,       "Wavelength/F");
	fThePhotonTreeDetected->Branch("OpChannel",       &fOpChannel,            "OpChannel/I");
	fThePhotonTreeDetected->Branch("Time",        &fTime,             "Time/F");
      }

    if(fMakeOpDetsTree)
      {
	fTheOpDetTree    = tfs->make<TTree>("OpDets","OpDets");
	fTheOpDetTree->Branch("EventID",        &fEventID,          "EventID/I");
	fTheOpDetTree->Branch("OpChannel",          &fOpChannel,            "OpChannel/I");
	fTheOpDetTree->Branch("CountAll",       &fCountOpDetAll,      "CountAll/I");
	fTheOpDetTree->Branch("CountDetected",  &fCountOpDetDetected, "CountDetected/I");
      }
    
    if(fMakeOpDetEventsTree)
      {
	fTheEventTree  = tfs->make<TTree>("OpDetEvents","OpDetEvents");
	fTheEventTree->Branch("EventID",      &fEventID,            "EventID/I");
	fTheEventTree->Branch("CountAll",     &fCountEventAll,     "CountAll/I");
	fTheEventTree->Branch("CountDetected",&fCountEventDetected,"CountDetected/I");
      }

  }

  
  SimPhotonCounter::~SimPhotonCounter() 
  {
  }
  
  void SimPhotonCounter::endJob()
  {
    art::ServiceHandle<phot::PhotonVisibilityService> vis;
   
    if(vis->IsBuildJob())
      {
	vis->StoreLibrary();
      }
  }

  void SimPhotonCounter::analyze(art::Event const& evt)
  {

    // Setup random number generator (for QE sampling)
    art::ServiceHandle<art::RandomNumberGenerator> rng;
    CLHEP::HepRandomEngine &engine = rng->getEngine();
    CLHEP::RandFlat   flat(engine);

    // Lookup event ID from event
    art::EventNumber_t event = evt.id().event();
    fEventID=Int_t(event);

    art::ServiceHandle<sim::LArG4Parameters> lgp;
    bool fUseLitePhotons = lgp->UseLitePhotons();

    if(!fUseLitePhotons)
    {
    //Get SimPhotonsCollection from Event
    sim::SimPhotonsCollection TheHitCollection = sim::SimListUtils::GetSimPhotonsCollection(evt,fInputModule);

    //Reset counters
    fCountEventAll=0;
    fCountEventDetected=0;

    if(fVerbosity > 0) std::cout<<"Found OpDet hit collection of size "<< TheHitCollection.size()<<std::endl;
    if(TheHitCollection.size()>0)
      {
	for(sim::SimPhotonsCollection::const_iterator itOpDet=TheHitCollection.begin(); itOpDet!=TheHitCollection.end(); itOpDet++)
	  {
	    //Reset Counters
	    fCountOpDetAll=0;
	    fCountOpDetDetected=0;

	    //Get data from HitCollection entry
	    fOpChannel=itOpDet->first;
	    const sim::SimPhotons& TheHit=itOpDet->second;
	     
	    //	    std::cout<<"OpDet " << fOpChannel << " has size " << TheHit.size()<<std::endl;
	    
	    // Loop through OpDet phots.  
	    //   Note we make the screen output decision outside the loop
	    //   in order to avoid evaluating large numbers of unnecessary 
	    //   if conditions. 

	    if(fVerbosity > 3)
	      {
		for(const sim::OnePhoton& Phot: TheHit)
		  {
		    // Calculate wavelength in nm
		    fWavelength= (2.0*3.142)*0.000197/Phot.Energy;

		    //Get arrival time from phot
		    fTime= Phot.Time;
		    std::cout<<"Arrival time: " << fTime<<std::endl;
		    
		    // Increment per OpDet counters and fill per phot trees
		    fCountOpDetAll++;
		    if(fMakeAllPhotonsTree) fThePhotonTreeAll->Fill();
		    if((flat.fire(1.0)<=fQE)&&(fWavelength>fWavelengthCutLow)&&(fWavelength<fWavelengthCutHigh))
		      {
			if(fMakeDetectedPhotonsTree) fThePhotonTreeDetected->Fill();
			fCountOpDetDetected++;
			std::cout<<"OpDetResponse PerPhoton : Event "<<fEventID<<" OpChannel " <<fOpChannel << " Wavelength " << fWavelength << " Detected 1 "<<std::endl;
		      }
		    else
		      std::cout<<"OpDetResponse PerPhoton : Event "<<fEventID<<" OpChannel " <<fOpChannel << " Wavelength " << fWavelength << " Detected 0 "<<std::endl;
		  }
	      }
	    else
	      {
		for(const sim::OnePhoton& Phot: TheHit)
		  {
		    // Calculate wavelength in nm
		    fWavelength= (2.0*3.142)*0.000197/Phot.Energy;
		    fTime= Phot.Time;		
    
		    // Increment per OpDet counters and fill per phot trees
		    fCountOpDetAll++;
		    if(fMakeAllPhotonsTree) fThePhotonTreeAll->Fill();
		    if((flat.fire(1.0)<=fQE)&&(fWavelength>fWavelengthCutLow)&&(fWavelength<fWavelengthCutHigh))
		      {
			if(fMakeDetectedPhotonsTree) fThePhotonTreeDetected->Fill();
			fCountOpDetDetected++;
		      }
		  }
	      }
	  
	  	      
	    // If this is a library building job, fill relevant entry
	    art::ServiceHandle<phot::PhotonVisibilityService> pvs;
	    if(pvs->IsBuildJob())
	      {
		int VoxID; double NProd;
		pvs->RetrieveLightProd(VoxID, NProd);
		pvs->SetLibraryEntry(VoxID, fOpChannel, double(fCountOpDetAll)/NProd);		
	      }


	    // Incremenent per event and fill Per OpDet trees	    
	    if(fMakeOpDetsTree) fTheOpDetTree->Fill();
	    fCountEventAll+=fCountOpDetAll;
	    fCountEventDetected+=fCountOpDetDetected;

	    // Give per OpDet output
	    if(fVerbosity >2) std::cout<<"OpDetResponse PerOpDet : Event "<<fEventID<<" OpDet " << fOpChannel << " All " << fCountOpDetAll << " Det " <<fCountOpDetDetected<<std::endl; 
	  }

	// Fill per event tree
	if(fMakeOpDetEventsTree) fTheEventTree->Fill();

	// Give per event output
	if(fVerbosity >1) std::cout<<"OpDetResponse PerEvent : Event "<<fEventID<<" All " << fCountOpDetAll << " Det " <<fCountOpDetDetected<<std::endl; 	

      }
    else
      {
	// if empty OpDet hit collection, 
	// add an empty record to the per event tree 
	if(fMakeOpDetEventsTree) fTheEventTree->Fill();
      }
    
    }
    else
    {
    //Get SimPhotonsLite from Event
    art::Handle< std::vector<sim::SimPhotonsLite> > photonHandle; 
    evt.getByLabel("largeant", photonHandle);

    
    //Reset counters
    fCountEventAll=0;
    fCountEventDetected=0;

    if(fVerbosity > 0) std::cout<<"Found OpDet hit collection of size "<< (*photonHandle).size()<<std::endl;

    
    if((*photonHandle).size()>0)
      {
        
        for ( auto const& photon : (*photonHandle) )
        {
          //Get data from HitCollection entry
          fOpChannel=photon.OpChannel;
          std::map<int, int> PhotonsMap = photon.DetectedPhotons;

          //Reset Counters
          fCountOpDetAll=0;
          fCountOpDetDetected=0;

	    if(fVerbosity > 3)
	      {
            for(auto it = PhotonsMap.begin(); it!= PhotonsMap.end(); it++)
            {
		    // Calculate wavelength in nm
		    fWavelength= 128;

		    //Get arrival time from phot
		    fTime= it->first*2;
		    std::cout<<"Arrival time: " << fTime<<std::endl;
		   
            for(int i = 0; i < it->second ; i++)
            {
		    // Increment per OpDet counters and fill per phot trees
		    fCountOpDetAll++;
		    if(fMakeAllPhotonsTree) fThePhotonTreeAll->Fill();
		    if((flat.fire(1.0)<=fQE)&&(fWavelength>fWavelengthCutLow)&&(fWavelength<fWavelengthCutHigh))
		      {
			if(fMakeDetectedPhotonsTree) fThePhotonTreeDetected->Fill();
			fCountOpDetDetected++;
			std::cout<<"OpDetResponse PerPhoton : Event "<<fEventID<<" OpChannel " <<fOpChannel << " Wavelength " << fWavelength << " Detected 1 "<<std::endl;
		      }
		    else
		      std::cout<<"OpDetResponse PerPhoton : Event "<<fEventID<<" OpChannel " <<fOpChannel << " Wavelength " << fWavelength << " Detected 0 "<<std::endl;
            }
            }
	      }
	    else
	      {
		    for(auto it = PhotonsMap.begin(); it!= PhotonsMap.end(); it++)
            {
		      // Calculate wavelength in nm
		      fWavelength= 128;
		      fTime= it->first*2;		
   
              for(int i = 0; i < it->second; i++)
              {
                // Increment per OpDet counters and fill per phot trees
                fCountOpDetAll++;
                if(fMakeAllPhotonsTree) fThePhotonTreeAll->Fill();
                if((flat.fire(1.0)<=fQE)&&(fWavelength>fWavelengthCutLow)&&(fWavelength<fWavelengthCutHigh))
		        {
                  if(fMakeDetectedPhotonsTree) fThePhotonTreeDetected->Fill();
                  fCountOpDetDetected++;
		        }
              }
            }
          }
	  	      
	    // Incremenent per event and fill Per OpDet trees	    
	    if(fMakeOpDetsTree) fTheOpDetTree->Fill();
	    fCountEventAll+=fCountOpDetAll;
	    fCountEventDetected+=fCountOpDetDetected;

	    // Give per OpDet output
	    if(fVerbosity >2) std::cout<<"OpDetResponse PerOpDet : Event "<<fEventID<<" OpDet " << fOpChannel << " All " << fCountOpDetAll << " Det " <<fCountOpDetDetected<<std::endl; 
        }
        // Fill per event tree
        if(fMakeOpDetEventsTree) fTheEventTree->Fill();

        // Give per event output
        if(fVerbosity >1) std::cout<<"OpDetResponse PerEvent : Event "<<fEventID<<" All " << fCountOpDetAll << " Det " <<fCountOpDetDetected<<std::endl; 	

      }
    else
    {
      // if empty OpDet hit collection, 
      // add an empty record to the per event tree 
      if(fMakeOpDetEventsTree) fTheEventTree->Fill();
    } 
    }
  }
}


namespace opdet{

  DEFINE_ART_MODULE(SimPhotonCounter)

}//end namespace opdet

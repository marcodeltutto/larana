////////////////////////////////////////////////////////////////////////
// Class:       BeamFlashTrackMatchTagger
// Module Type: producer
// File:        BeamFlashTrackMatchTagger_module.cc
// Author:      Wes Ketchum, based on code from Ben Jones
// 
// Description: Module that compares all tracks to the flash during the 
//              beam gate, and determines if that track is consistent with
//              having produced that flash.
// Input:       recob::OpFlash, recob::Track
// Output:      anab::CosmicTag (and Assn<anab::CosmicTag,recob::Track>)
//
// Generated at Sat Aug 16 15:57:53 2014 by Wesley Ketchum using artmod
// from cetpkgsupport v1_06_02.
////////////////////////////////////////////////////////////////////////

#include "art/Framework/Core/EDProducer.h"
#include "art/Framework/Core/ModuleMacros.h"
#include "art/Framework/Principal/Event.h"
#include "art/Framework/Principal/Handle.h"
#include "art/Framework/Principal/Run.h"
#include "art/Framework/Principal/SubRun.h"
#include "canvas/Utilities/InputTag.h"
#include "fhiclcpp/ParameterSet.h"
#include "messagefacility/MessageLogger/MessageLogger.h"

#include <memory>

#include "larcore/Geometry/Geometry.h"
#include "lardata/Utilities/AssociationUtil.h"
#include "lardata/DetectorInfoServices/ServicePack.h" // lar::extractProviders()
#include "lardata/DetectorInfoServices/LArPropertiesService.h"
#include "lardataobj/RecoBase/Hit.h"
#include "BeamFlashTrackMatchTaggerAlg.h"
#include "HitTagAssociatorAlg.h"

namespace cosmic {
  class BeamFlashTrackMatchTagger;
}

class cosmic::BeamFlashTrackMatchTagger : public art::EDProducer {
public:
  explicit BeamFlashTrackMatchTagger(fhicl::ParameterSet const & p);
  // The destructor generated by the compiler is fine for classes
  // without bare pointers or other resource use.

  // Plugins should not be copied or assigned.
  BeamFlashTrackMatchTagger(BeamFlashTrackMatchTagger const &) = delete;
  BeamFlashTrackMatchTagger(BeamFlashTrackMatchTagger &&) = delete;
  BeamFlashTrackMatchTagger & operator = (BeamFlashTrackMatchTagger const &) = delete;
  BeamFlashTrackMatchTagger & operator = (BeamFlashTrackMatchTagger &&) = delete;
  void reconfigure(fhicl::ParameterSet const& p);
  void produce(art::Event & e) override;


private:

  // Declare member data here.
  BeamFlashTrackMatchTaggerAlg fAlg;
  std::string fTrackModuleLabel;
  std::string fFlashModuleLabel;
  
  HitTagAssociatorAlg fHitTagAssnsAlg;
  bool fMakeHitTagAssns;
  std::string fHitModuleLabel;

};


cosmic::BeamFlashTrackMatchTagger::BeamFlashTrackMatchTagger(fhicl::ParameterSet const & p)
  : EDProducer{p},
    fAlg(p.get<fhicl::ParameterSet>("BeamFlashTrackMatchAlgParams")),
    fTrackModuleLabel(p.get<std::string>("TrackModuleLabel")),
    fFlashModuleLabel(p.get<std::string>("FlashModuleLabel")),
    fHitTagAssnsAlg(p.get<fhicl::ParameterSet>("HitTagAssociatorAlgParams")),
    fMakeHitTagAssns(p.get<bool>("MakeHitTagAssns",false)),
    fHitModuleLabel(p.get<std::string>("HitModuleLabel","dummy_hit"))
{
  produces< std::vector<anab::CosmicTag> >();
  produces< art::Assns<recob::Track, anab::CosmicTag> >();
  if(fMakeHitTagAssns) produces< art::Assns<recob::Hit, anab::CosmicTag> >();
}

void cosmic::BeamFlashTrackMatchTagger::reconfigure(fhicl::ParameterSet const& p){
  fhicl::ParameterSet alg_params = p.get<fhicl::ParameterSet>("BeamFlashTrackMatchAlgParams");
  fAlg.reconfigure(alg_params);

  fhicl::ParameterSet hittagassnsalg_params = p.get<fhicl::ParameterSet>("HitTagAssociatorAlgParams");
  fHitTagAssnsAlg.reconfigure(hittagassnsalg_params);

  fTrackModuleLabel = p.get<std::string>("TrackModuleLabel");   
  fFlashModuleLabel = p.get<std::string>("FlashModuleLabel");
  fMakeHitTagAssns  = p.get<bool>("MakeHitTagAssns",false);
  fHitModuleLabel   = p.get<std::string>("HitModuleLabel","dummy_hit");

}

void cosmic::BeamFlashTrackMatchTagger::produce(art::Event & evt)
{
  // services and providers we'll be using
  auto providers = lar::extractProviders<
    geo::Geometry,
    detinfo::LArPropertiesService
    >();
  
  art::ServiceHandle<phot::PhotonVisibilityService> pvsHandle;
  phot::PhotonVisibilityService const& pvs(*pvsHandle);
  art::ServiceHandle<opdet::OpDigiProperties> opdigipHandle;
  opdet::OpDigiProperties const& opdigip(*opdigipHandle);

  //Get Flashes from event.
  art::Handle< std::vector<recob::OpFlash> > flashHandle;
  evt.getByLabel(fFlashModuleLabel, flashHandle);
  std::vector<recob::OpFlash> const& flashVector(*flashHandle);

  //Get Tracks from event.
  art::Handle< std::vector<recob::Track> > trackHandle;
  evt.getByLabel(fTrackModuleLabel, trackHandle);
  std::vector<recob::Track> const& trackVector(*trackHandle);

  //Make the containger for the tag product to put onto the event.
  std::unique_ptr< std::vector<anab::CosmicTag> > cosmicTagPtr ( new std::vector<anab::CosmicTag>);
  std::vector<anab::CosmicTag> & cosmicTagVector(*cosmicTagPtr);
  
  //Make a container for the track<-->tag associations. 
  //One entry per track, with entry equal to index in cosmic tag collection of associated tag.
  std::vector<size_t> assnTrackTagVector;
  std::unique_ptr< art::Assns<recob::Track,anab::CosmicTag> > assnTrackTag(new art::Assns<recob::Track,anab::CosmicTag>);
  
  //run the alg!
  fAlg.RunCompatibilityCheck(flashVector, trackVector, 
			     cosmicTagVector, assnTrackTagVector,
			     providers, pvs, opdigip);


  //Make the associations for ART
  for(size_t track_iter=0; track_iter<assnTrackTagVector.size(); track_iter++){
    if(assnTrackTagVector[track_iter]==std::numeric_limits<size_t>::max()) continue;
    art::Ptr<recob::Track> trk_ptr(trackHandle,track_iter);
    util::CreateAssn(*this, evt, cosmicTagVector, trk_ptr, *assnTrackTag, assnTrackTagVector[track_iter]); 
  }
  
  //make hit<--> tag associations, if requested
  if(fMakeHitTagAssns){

    //Get Hits from event.
    art::Handle< std::vector<recob::Hit> > hitHandle;
    evt.getByLabel(fHitModuleLabel, hitHandle);

    //Get track<-->hit associations
    art::Handle< art::Assns<recob::Hit,recob::Track> > assnHitTrackHandle;
    evt.getByLabel(fTrackModuleLabel,assnHitTrackHandle);
    std::vector< std::vector<size_t> > 
      track_indices_per_hit = util::GetAssociatedVectorManyI(assnHitTrackHandle,
							     hitHandle);

    std::vector< std::vector<size_t> > assnHitTagVector;
    std::unique_ptr< art::Assns<recob::Hit,anab::CosmicTag> > assnHitTag(new art::Assns<recob::Hit,anab::CosmicTag>);

    fHitTagAssnsAlg.MakeHitTagAssociations(track_indices_per_hit,
					   assnTrackTagVector,
					   assnHitTagVector);

    //Make the associations for ART
    for(size_t hit_iter=0; hit_iter<assnHitTagVector.size(); hit_iter++){
      art::Ptr<recob::Hit> hit_ptr(hitHandle,hit_iter);
      for(size_t tag_iter=0; tag_iter<assnHitTagVector[hit_iter].size(); tag_iter++)
	util::CreateAssn(*this, evt, cosmicTagVector, hit_ptr, *assnHitTag, assnHitTagVector[hit_iter][tag_iter]); 
    }

    evt.put( std::move(assnHitTag));
  }//end if makes hit<-->tag associations
  
  //put the data on the event
  evt.put(std::move(cosmicTagPtr));
  evt.put(std::move(assnTrackTag));

}

DEFINE_ART_MODULE(cosmic::BeamFlashTrackMatchTagger)

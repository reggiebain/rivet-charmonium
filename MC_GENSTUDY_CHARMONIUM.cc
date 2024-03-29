// -*- C++ -*-

//System 
#include <map>
#include <algorithm>
#include <cmath>

// Rivet 
#include "Rivet/Analysis.hh"
#include "Rivet/AnalysisLoader.hh"
//#include "Rivet/RivetAIDA.hh"

//Projections
#include "Rivet/Projections/FinalState.hh"
#include "Rivet/Projections/ChargedLeptons.hh"
#include "Rivet/Projections/FastJets.hh"

typedef std::map<std::string,Rivet::Histo1DPtr> BookedHistos;
template <typename lvec> static void dump4vec(lvec four_mom){
  std::cout<<"( "<<four_mom.pt()<<" [GeV], "<<four_mom.eta()<<", "<<four_mom.phi()<<", "<<four_mom.mass()<<" [GeV])"<<std::endl;
}
namespace Rivet {


  /// Generic analysis looking at various distributions of final state particles
  class MC_GENSTUDY_CHARMONIUM : public Analysis {
  public:

    /// Constructor
    MC_GENSTUDY_CHARMONIUM()
      : Analysis("MC_GENSTUDY_CHARMONIUM"),
	jetR(0.4)
    {    }


  public:

    /// @name Analysis methods
    //@{

    /// Book histograms and initialise projections before the run
    void init() {

      // Projections
      const FinalState fs(-4.2, 4.2, .5*GeV);
      addProjection(fs, "FS");
      ChargedLeptons lfs(fs);
      addProjection(lfs, "LFS");
      FastJets JetProjection(fs,FastJets::ANTIKT, jetR);
      addProjection(JetProjection,"Jets");

      // Histograms
      _histograms["JetPt"] = bookHisto1D("JetPt" , 50, 0, 20);
      _histograms["JetM"] = bookHisto1D("JetM" , 25, 0, 11);
      _histograms["JetEta"] = bookHisto1D("JetEta" , 25, -3, 3);
      _histograms["JetMult"] = bookHisto1D("JetMult",40,-0.5,40.5);

      _histograms["JPsiPt"] = bookHisto1D("JPsiPt" , 50, 0, 20);
      _histograms["JPsiM"] = bookHisto1D("JPsiM" , 50, 3.05, 3.15);
      _histograms["JPsiEta"] = bookHisto1D("JPsiEta" , 25, -3, 3);

      // Substructure variables
      _histograms["DeltaR"] = bookHisto1D("DeltaR",50,0,jetR+0.1);
      _histograms["JetZ"] = bookHisto1D("JetZ",50,0,1.05);
    }



    /// Perform the per-event analysis
    void analyze(const Event& event) {
      cutFlow["Nominal"]++;
      const double weight = event.weight();
      const ChargedLeptons& lProj = applyProjection<ChargedLeptons>(event, "LFS");
      if(lProj.chargedLeptons().empty()){
	vetoEvent;
      }
      cutFlow["Leptons"]++;

      Particles muons;
      foreach(const Particle& lepton,lProj.chargedLeptons()){
	if(abs(lepton.pid())==13){
	  muons.push_back(lepton);
	}
      }
      if(muons.size() < 2){
	vetoEvent;
      }
      cutFlow["2Muons"]++;

      FourMomentum j_psi;
      FourMomentum cand;
      double deltaM=10000.;
      const double j_psi_m = 3.096916;
      foreach(const Particle& mu1, muons){
	foreach(const Particle& mu2, muons){
	  cand=mu1.momentum()+mu2.momentum();
	  if(mu1.pid()*mu2.pid() < 0 && cand.mass()-j_psi_m < deltaM ){
	    j_psi=cand;
	    deltaM=cand.mass()-j_psi_m;
	  }
	}
      }
      if(j_psi.mass()==0) {
	vetoEvent;
      }
      // dump4vec(j_psi);
      const FastJets& jetProj = applyProjection<FastJets>(event, "Jets");
      const Jets jets = jetProj.jetsByPt(1*GeV);
      if(jets.empty()){
	vetoEvent;
      }
      cutFlow["Jets"]++;
      _histograms["JetMult"]->fill(jets.size(),weight);

      //Process the particles

      //FourMomentum j_psi=muons[0].momentum()+muons[1].momentum();

      //fill j_psi histos
      _histograms["JPsiEta"]->fill(j_psi.eta(),weight);
      _histograms["JPsiPt"]->fill(j_psi.pt(),weight);
      _histograms["JPsiM"]->fill(j_psi.mass(),weight);

      Jet charmJet;
      double delR(99.);
      double candDelR(99.);
      foreach(const Jet& j, jets){
	delR=deltaR(j.momentum(), j_psi);
      	if( delR < jetR && delR < candDelR) {
      	  charmJet=j;
	  candDelR=delR;
      	}
      }
      if(isinf(deltaR(charmJet,j_psi))){
	vetoEvent;
      }
      cutFlow["charmJetMatch"]++;
      if(isinf(deltaR(charmJet,j_psi))) {
      	vetoEvent;
      }

      _histograms["DeltaR"]->fill(deltaR(j_psi,charmJet),weight);
      _histograms["JetPt"]->fill(charmJet.pt(),weight);
      _histograms["JetM"]->fill(charmJet.mass(),weight);
      _histograms["JetEta"]->fill(charmJet.eta(),weight);


      //calculate substructure variables
      const double z(charmJet.pt()+j_psi.pt() > 0 ? j_psi.pt()/(charmJet.pt() + j_psi.pt()) : -1.);
      
      //fill substructure histos
      _histograms["JetZ"]->fill(z,weight);
    }

    /// Finalize
    void finalize() {
      cout<<"Cut flow"<<endl;
      cout<<"|-"<<endl;
      for(std::map<std::string, size_t>::const_iterator cut = cutFlow.begin();
	  cut != cutFlow.end(); ++cut){
	cout<<"| "<<cut->first << " | "<<cut->second<<" |"<<endl;
      }
      cout<<"|-"<<endl;
    }

    //@}


  private:
    double jetR;
    /// @name Histograms
    //@{
    BookedHistos _histograms;
    //@}
    std::map<std::string, size_t> cutFlow;
    
  };


  // The hook for the plugin system
  DECLARE_RIVET_PLUGIN(MC_GENSTUDY_CHARMONIUM);

}

/**
 * @file MultiPhase.h
 * Headers for the \link Cantera::MultiPhase MultiPhase\endlink 
 * object that is used to set up multiphase equilibrium problems (see \ref equilfunctions).
 *
 */
/*
 *  $Author$
 *  $Date$
 *  $Revision$
 */
#ifndef CT_MULTIPHASE_H
#define CT_MULTIPHASE_H

#include "ct_defs.h"
#include "DenseMatrix.h"
#include "ThermoPhase.h"

namespace Cantera {

  //! A class for multiphase mixtures. The mixture can contain any
  //! number of phases of any type. 
  /*!
   *    All phases have the same
   *    temperature and pressure, and a specified number of moles.
   *    The phases do not need to have the same elements. For example,
   *    a mixture might consist of a gaseous phase with elements (H,
   *    C, O, N), a solid carbon phase containing only element C,
   *    etc. A master element set will be constructed for the mixture
   *    that is the union of the elements of each phase.
   *
   *  @ingroup equilfunctions
   */
  class MultiPhase {

  public:

    //! Shorthand for an index variable that can't be negative
    typedef size_t       index_t;

    //! Shorthand for a ThermoPhase
    typedef ThermoPhase  phase_t;

    //! shorthand for a 2D matrix
    typedef DenseMatrix  array_t;

    //! Shorthand for a vector of pointers to ThermoPhase's
    typedef std::vector<phase_t*> phase_list;

    /// Constructor. The constructor takes no arguments, since
    /// phases are added using method addPhase.
    MultiPhase();

    /// Destructor. Does nothing. Class MultiPhase does not take
    /// "ownership" (i.e. responsibility for destroying) the
    /// phase objects.
    virtual ~MultiPhase() {}

    //! Add a vector of phases to the mixture
    /*!
     * See the single addPhases command. This just does a bunch of phases
     * at one time
     *   @param phases Vector of pointers to phases
     *   @param phaseMoles Vector of mole numbers in each phase (kmol)
     */
    void addPhases(phase_list& phases, const vector_fp& phaseMoles);

    /// Add all phases present in 'mix' to this mixture.
    void addPhases(MultiPhase& mix);

    /// Add a phase to the mixture.
    /// @param p pointer to the phase object
    /// @param moles total number of moles of all species in this phase
    void addPhase(phase_t* p, doublereal moles);

    /// Number of elements.
    int nElements() const { return int(m_nel); }

    /// Name of element \a m.
    std::string elementName(int m) const;

    /// Index of element with name \a name.
    int elementIndex(std::string name) const;

    //! Number of species, summed over all phases.
    int nSpecies() const { return int(m_nsp); }

    //! Name of species with global index \a k.
    std::string speciesName(int kGlob) const;

    /// Number of atoms of element \a m in species \a k.
    doublereal nAtoms(int kGlob, int mGlob) {
      if (!m_init) init();
      return m_atoms(mGlob, kGlob);
    }

    /// Species mole fractions. Write the array of species mole
    /// fractions into array \c x. The mole fractions are
    /// normalized to sum to one in each phase.
    void getMoleFractions(doublereal* x) const {
      std::copy(m_moleFractions.begin(), m_moleFractions.end(), x);
    }

    //! Process phases and build atomic composition array. 
    /*!This method
     *  must be called after all phases are added, before doing
     *  anything else with the mixture. After init() has been called,
     *  no more phases may be added.
     */
    void init();

    //! Return the number of moles in phase n.
    /*!
     * @param n  Index of the phase.
     */
    doublereal phaseMoles(index_t n) const {
      return m_moles[n];
    }

    //! Set the number of moles of phase with index n.
    /*!
     * @param n     Index of the phase
     * @param moles Number of moles in the phase (kmol)
     */
    void setPhaseMoles(index_t n, doublereal moles) {
      m_moles[n] = moles;
    }

    /// Return a %ThermoPhase reference to phase n.
    /*! The state of phase n is
     *  also updated to match the state stored locally in the
     *  mixture object.
     *
     * @param n  Phase Index
     *
     * @return   Reference to the %ThermoPhase object for the phase
     */
    phase_t& phase(index_t n);

    //! Returns the moles of global species \c k.
    /*!
     * Returns the moles of global species k.
     * units = kmol
     *
     * @param kGlob   Global species index k
     */
    doublereal speciesMoles(index_t kGlob) const;

    /// Index of the species belonging to phase number \c p
    /// with index \c k within the phase.
    int speciesIndex(index_t k, index_t p) const {
      return m_spstart[p] + k;
    }

    /// Minimum temperature for which all solution phases have
    /// valid thermo data. Stoichiometric phases are not
    /// considered, since they may have thermo data only valid for
    /// conditions for which they are stable.
    doublereal minTemp() const { return m_Tmin; }

    /// Maximum temperature for which all solution phases have
    /// valid thermo data. Stoichiometric phases are not
    /// considered, since they may have thermo data only valid for
    /// conditions for which they are stable.
    doublereal maxTemp() const { return m_Tmax; }

    /// Total charge (Coulombs).
    doublereal charge() const;

    /// Charge (Coulombs) of phase with index \a p.
    doublereal phaseCharge(index_t p) const;

    /// Total moles of element \a m, summed over all phases.
    doublereal elementMoles(index_t m) const;

    /// Chemical potentials. Write into array \a mu the chemical
    /// potentials of all species [J/kmol]. The chemical
    /// potentials are related to the activities by
    /// \f[ \mu_k = \mu_k^0(T, P) + RT \ln a_k. \f].
    void getChemPotentials(doublereal* mu) const;

    /// Valid chemical potentials. Write into array \a mu the
    /// chemical potentials of all species with thermo data valid
    /// for the current temperature [J/kmol]. For other species,
    /// set the chemical potential to the value \a not_mu. If \a
    /// standard is set to true, then the values returned are
    /// standard chemical potentials.
    void getValidChemPotentials(doublereal not_mu, doublereal* mu,
				bool standard = false) const;

    /// Temperature [K].
    doublereal temperature() const { return m_temp; }

    //! Set the mixture to a state of chemical equilibrium.
    /*!
     *    @param XY   Integer flag specifying properties to hold fixed.
     *    @param err  Error tolerance for \f$\Delta \mu/RT \f$ for
     *                all reactions. Also used as the relative error tolerance
     *                for the outer loop.
     *    @param maxsteps Maximum number of steps to take in solving
     *                    the fixed TP problem.
     *    @param maxiter Maximum number of "outer" iterations for
     *                   problems holding fixed something other than (T,P).
     *    @param loglevel Level of diagnostic output, written to a
     *                    file in HTML format.
     */
    doublereal equilibrate(int XY, doublereal err = 1.0e-9,
			   int maxsteps = 1000, int maxiter = 200, int loglevel = -99);


    /// Set the temperature [K].
    void setTemperature(doublereal T) {
      m_temp = T;
      updatePhases();
    }

    /// Pressure [Pa].
    doublereal pressure() const {
      return m_press;
    }

    /// Volume [m^3].
    doublereal volume() const;

    /// Set the pressure [Pa].
    void setPressure(doublereal P) {
      m_press = P;
      updatePhases();
    }

    /// Enthalpy [J].
    doublereal enthalpy() const;

    /// Entropy [J/K].
    doublereal entropy() const;

    /// Gibbs function [J].
    doublereal gibbs() const;

    /// Heat capacity at constant pressure [J/K].
    doublereal cp() const;

    /// Number of phases.
    index_t nPhases() const {
      return m_np;
    }

    /// Return true is species \a kGlob is a species in a
    /// multicomponent solution phase.
    bool solutionSpecies(index_t kGlob) const;

    //! Returns the phase index of the Kth "global" species
    /*!
     * @param kGlob Global species index.
     *
     * @return
     *     Returns the index of the owning phase.
     */
    index_t speciesPhaseIndex(index_t kGlob) const {
      return m_spphase[kGlob];
    }

    //! Returns the mole fraction of global species k
    doublereal moleFraction(index_t kGlob) const{
      return m_moleFractions[kGlob];
    }

    void setPhaseMoleFractions(index_t n, doublereal* x);

    void setMolesByName(compositionMap& xMap);

    //! Set the Moles via a string containing their names.
    /*!
     * The string x is in the form of a composition map
     * Species which are not listed by name in the composition
     * map are set to zero.
     *
     * @param x string x in the form of a composition map
     *             where values are the moles of the species.
     */
    void setMolesByName(const std::string& x);

  
    //! Return a vector of global species mole numbers
    /*!
     *  Returns a vector of the number of moles of each species
     *  in the multiphase object.
     *
     * @param molNum Vector of doubles of length nSpecies
     *               containing the global mole numbers
     *               (kmol).
     */
    void getMoles(doublereal * molNum) const;

    //! Sets all of the global species mole numbers
    /*!
     *  Sets the number of moles of each species
     *  in the multiphase object.
     *
     * @param n    Vector of doubles of length nSpecies
     *             containing the global mole numbers
     *               (kmol).
     */
    void setMoles(doublereal* n);

    //! Retrieves a vector of element abundances
    /*!
     * @param elemAbundances  Vector of element abundances
     * Length = number of elements in the MultiPhase object.
     * Index is the global element index
     * units is in kmol.
     */
    void getElemAbundances(doublereal * elemAbundances) const;

    //! Return true if the phase \a p has valid thermo data for
    //! the current temperature.
    /*!
     * @param p  Index of the phase.
     */
    bool tempOK(index_t p) const {
      return m_temp_OK[p];
    }


    // These methods are meant for internal use.

    /// update the locally-stored composition to match the current
    /// compositions of the phase objects.
    void updateMoleFractions();

  protected:
    /// Set the states of the phase objects to the locally-stored
    /// state.  Note that if individual phases have T and P different
    /// than that stored locally, the phase T and P will be modified.
    void updatePhases() const;

    //! Calculate the element abundance vector
    void calcElemAbundances() const;
    /**
     * Vector of the number of moles in each phase.
     * Length = m_np, number of phases.
     */
    vector_fp m_moles;

    /**
     * Vector of the ThermoPhase Pointers.
     */
    std::vector<phase_t*> m_phase;

    //! Global Stoichiometric Coefficient array
    /*!
     *  This is a two dimensional array m_atoms(m, k). The first
     *  index is the global element index. The second index, k, is the
     *  global species index.
     *  The value is the number of atoms of type m in species k.
     */
    array_t m_atoms;

    /**
     * Locally storred vector of mole fractions of all species
     * comprising the MultiPhase object.
     */
    vector_fp m_moleFractions;
    vector_int m_spphase;

    //! Vector of ints containing of first species index in the global list of species
    //! for each phase
    /*!
     *  kfirst = m_spstart[ip], kfirst is the index of the first species in the ip'th
     *                          phase.
     */
    vector_int m_spstart;
    std::vector<std::string> m_enames;
    vector_int m_atomicNumber;

    //! Vector of species names in the problem
    /*!
     *   Vector is over all species defined in the object,
     *   the global species index.
     */
    std::vector<std::string> m_snames;

    mutable std::map<std::string, int> m_enamemap;
    /**
     *   Number of phases in the MultiPhase object
     */
    index_t  m_np;
    doublereal m_temp;
    doublereal m_press;
    /**
     * Number of distinct elements in all of the phases
     */
    index_t m_nel;
    /**
     * Number of distinct species in all of the phases
     */
    index_t m_nsp;
    bool m_init;
    int m_eloc;

    //! Vector of bools indicating whether temperatures are ok for phases.
    /*!
     * If the current temperature is outside the range of valid temperatures
     * for the phase thermodynamics, the phase flag is set to false.
     */
    mutable std::vector<bool> m_temp_OK;

    //! Minimum temperature for which thermo parameterizations are valid
    /*!
     *  Stoichiometric phases are ignored in this determination.
     *  units Kelvin
     */
    doublereal m_Tmin;

    //! Minimum temperature for which thermo parameterizations are valid
    /*!
     *  Stoichiometric phases are ignored in this determination.
     *  units Kelvin
     */
    doublereal m_Tmax;

    mutable vector_fp m_elemAbundances;
  };

  inline std::ostream& operator<<(std::ostream& s, Cantera::MultiPhase& x) {
    size_t ip;
    for (ip = 0; ip < x.nPhases(); ip++) {
      if (x.phase(ip).name() != "") {
	s << "*************** " << x.phase(ip).name() << " *****************" << std::endl;
      }
      else {
	s << "*************** Phase " << ip << " *****************" << std::endl;
      }
      s << "Moles: " << x.phaseMoles(ip) << std::endl;

      s << report(x.phase(ip)) << std::endl;
    }
    return s;
  }


  int BasisOptimize( int *usedZeroedSpecies, bool doFormRxn,
		     MultiPhase *mphase, vector_int & orderVectorSpecies,
		     vector_int & orderVectorElements,
		     vector_fp & formRxnMatrix);

  int ElemRearrange(int nComponents,  const vector_fp & elementAbundances,
		    MultiPhase *mphase,
		    vector_int & orderVectorSpecies,
		    vector_int & orderVectorElements);
#ifdef DEBUG_HKM
  extern int BasisOptimize_print_lvl;
#endif
}

#endif

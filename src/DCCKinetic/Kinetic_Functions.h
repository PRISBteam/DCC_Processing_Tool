///================================ A part of the DCC Kinetic module =============================================================///
///=================================================================================================================================///
/** The library contains random and non-random functions for choice and generation of new identifications for k-Cells               *
 *  in the DCC Kinetic module. It makes them "special" and takes out of the set of "ordinary" k-Cells.                            **/
///================================================================================================================================///

int DCC_Kinetic_Wear(double ShearStress, vector<Tup> &Grain_Orientations, Eigen::SparseMatrix<double> const& FES, std::vector<unsigned int> &CellNumbs, char* input_folder, char* output_dir) {
/// Vectors
    /// Normals - reading from file
    string Norm_path = input_folder + "Normal.txt"s;
    char* normals = const_cast<char*>(Norm_path.c_str());
    /// Calculation of the NormalForce for each Face
    std::vector<Tup> NormalForce;
    std::vector<Tup> Norms_tuple = TuplesReader(normals); // Reading from file Normal vectors of all Faces
    for (unsigned long ik = 0; ik < CellNumbs.at(2); ++ik) NormalForce.push_back( { ShearStress*get<0>(Norms_tuple[ik]), ShearStress*get<1>(Norms_tuple[ik]),ShearStress*get<2>(Norms_tuple[ik]) } );
    /// Set grain orientations (Random + From file)
    // I. Random case
    // The function initialize random seed from the computer time (MUST BE BEFORE THE FOR LOOP!)
    srand((unsigned) time(NULL));

    cout << "Hello there!" << endl;
    return 1;
}

///*============================================================================================*///
///*============================== DCC_Kinetic_Plasticity function =============================*///

vector<Tup> DCC_Kinetic_Plasticity(Eigen::SparseMatrix<double> const& FES, std::vector<unsigned int> &CellNumbs, char* input_folder, char* output_dir)  {
    //resultant tuple
    vector<Tup> fraction_stress_temperature;
/// Functions
    vector<unsigned int> Metropolis(vector<vector<double>> &stress_tensor, vector<vector<double>> &norms_vector, vector<vector<double>> &tang_vector, double &Temperature, std::vector<unsigned int> &CellNumbs, long iteration_number, vector<double> &slip_vector, double alpha, double lambda);
    vector<vector<double>> lt_vector(vector<vector<double>> &stress_tensor, vector<vector<double>> norms_vector);

/// Variables
    vector<unsigned int> State_Vector(CellNumbs.at(2),0); // State vector filling with zeros
    /// Other parameters
    vector<vector<double>> tang_vector; // tangential vector to the slip plane (lt)

    /// Normals - reading from file
    vector<vector<double>> norms_vector; // vector of normals
    string Norm_path = input_folder + "normals.txt"s;
    char* normals = const_cast<char*>(Norm_path.c_str());
    // Calculation of the NormalForce for each Face
    std::vector<Tup> Norms_tuple = TuplesReader(normals); // Reading from file Normal vectors of all Faces; TuplesReader() defined in DCC_SupportFunctions.h
    for (unsigned long ik = 0; ik < CellNumbs.at(2); ++ik) norms_vector.push_back( { get<0>(Norms_tuple[ik]), get<1>(Norms_tuple[ik]), get<2>(Norms_tuple[ik]) } );

    /// Slip areas for each Face - reading from file
    vector<double> SAreas, SAreas_m2; // vector of areas : unit + dimension
    string SAreas_path = input_folder + "faces_areas.txt"s;
    char* areas = const_cast<char*>(SAreas_path.c_str());
    SAreas = dVectorReader(areas); // reading from file; dVectorReader() defined in DCC_SupportFunctions.h

    /// Iteration over ShearStresses :: Input parameters
    double ShearStress = 0.0;
    double ShearStress_min = 0.1*pow(10,9), ShearStress_max = 20.0*pow(10,9);
    int simulation_steps = 10000, dSS = 0;
    /// Iteration over Temperature :: Input parameters
    double Temperature = 300.0;
    /// Material parameters
    double a_latticeCu = 3.628*pow(10,-10); // lattice parameter for bulk COPPER
    double BurgvCu = 2.56*pow(10,-10); // dislocation Burgers vector for COPPER
    double ShearModCu = 46.0*pow(10,9); // Shear modulus for COPPER

    double a_latticeAl = 4.0479*pow(10,-10); // lattice parameter for bulk ALUMINIUM
    double ShearModAl = 26.0*pow(10,9); // Shear modulus for ALUMINIUM
    double BurgvAl = 2.86*pow(10,-10); // Shear modulus for ALUMINIUM

    /// Model parameters
    double Burgv = BurgvCu, ShMod = ShearModCu, a_lattice = a_latticeCu;
    double lambda = 0.0*46.0*pow(10,9), //Shear modulus
             alpha = 0.5*ShMod; /// model coefficient alpha*s^2
    double D_size = 3.0*3.0*a_latticeCu; /// Complex size ///
    vector<double> external_normal = {0.0, 0.0, 1.0}; /// normal to the external face
    /// From unit areas to the real ones [m^2]
    for(auto slareas : SAreas) SAreas_m2.push_back(slareas*pow(a_lattice,2));

    ShearStress = ShearStress_min;
    dSS = (ShearStress_max - ShearStress_min)/ (double) simulation_steps;
    for (long i = 0; i < simulation_steps; ++i) {
        ShearStress += dSS;
        /// Stress tensor definition
        double s11 = 0.0, s12 = 0.0, s13 = 0.0, s21 = 0.0, s22 = 0.0, s23 = 0.0, s31 = 0.0, s32 = 0.0, s33 = ShearStress;
        vector<vector<double>> stress_tensor = {{s11, s12, s13}, {s21, s22, s23}, {s31, s32, s33} };
        /// OBTAINING OF NORMAL VECTORS
        tang_vector = lt_vector(stress_tensor, norms_vector);
         //for (auto p : tang_vector) cout << p[0] << "\t" << p[1] << "\t" << p[2] << endl; exit(32);
        /// Slip vectors
        vector<double> slip_vector; // vector of nano-slips mudulus (s)
        for(auto sa : SAreas_m2) slip_vector.push_back(Burgv * sqrt(M_PI/sa)); // nano-slip value s = b * Sqrt(Pi/As)
            //for (auto p : slip_vector) cout << "slip_vector" << SAreas_m2.size() << endl;

        /// METROPOLIS algorithm
        State_Vector.clear();

        /// Iteration number for METROPOLIS algorithm
        long iteration_number = 4.0 * SAreas.size();

        /// =========== Metropolis algorithm ============>>>
        State_Vector = Metropolis(stress_tensor, norms_vector, tang_vector, Temperature, CellNumbs, iteration_number, slip_vector, alpha, lambda); // Metropolis() is defined below

    /// Analysis :: slip_fraction and fraction_stress_temperature
        double slip_fraction = std::count(State_Vector.begin(), State_Vector.end(), 1)/ (double) State_Vector.size();
        fraction_stress_temperature.push_back(make_tuple(slip_fraction, ShearStress, Temperature));

    /// Plastic strain
        double Plastic_Strain = 0.0; unsigned int state_it = 0;
        for(auto state : State_Vector) {
            if (state == 1) {
               Plastic_Strain += SAreas_m2.at(state_it) * Burgv * abs(tang_vector.at(state_it)[0]*external_normal[0] + tang_vector.at(state_it)[1]*external_normal[1] + tang_vector.at(state_it)[2]*external_normal[2])/ pow(D_size, 3);
//                        inner_product(tang_vector.at(state_it).begin(), tang_vector.at(state_it).end(), external_normal.begin(), 0);
                          //  cout << "0:\t" << tang_vector.at(state_it)[0]*external_normal[0] << "\t1:\t" << tang_vector.at(state_it)[1]*external_normal[1] <<"\t2:\t" << tang_vector.at(state_it)[2]*external_normal[2] << endl;
            }
           ++state_it;
        }
        /// Output and stop!
        if (Plastic_Strain >= 0.002) { cout << "Plastic strain =\t" << Plastic_Strain << "\tYield strength [GPa] =\t" << ShearStress/pow(10,9) << endl; return fraction_stress_temperature;}
      //  cout << "\tSlip fraction =\t" << slip_fraction << "\tPlastic strain =\t" << Plastic_Strain << "\tYield strength [GPa] =\t" << ShearStress/pow(10,9) << endl;

    } // end of for (< calculation_steps)

    return fraction_stress_temperature;
}

vector<unsigned int> Metropolis(vector<vector<double>> &stress_tensor, vector<vector<double>> &norms_vector, vector<vector<double>> &tang_vector, double &Temperature, std::vector<unsigned int> &CellNumbs, long iteration_number, vector<double> &slip_vector, double alpha, double lambda){
    /// I. Constant initial state initialisation
    //zero vectors required for Processing_Random() input
    std::vector <unsigned int> SlipState_Vector(CellNumbs.at(2),0), s_faces_sequence(CellNumbs.at(2),0);
    double Rc = 8.31; //gas constant

    // ================> Initial p = 0.5 Face seeds
    Processing_Random(SlipState_Vector, s_faces_sequence, 0.5, 1, CellNumbs);

    // 1. Loop over all the Faces
    srand((unsigned) time(NULL)); // The function initialize random seed from the computer time (MUST BE BEFORE THE FOR LOOP!)

    for (long i = 0; i < iteration_number; ++i) {
    // 2. Random choice of a new Face
    long NewSlipNumber = rand() % (CellNumbs.at(2)-2) ; // Random generation of the boundary number in the range from 0 to CellNumbs.at(2)

    // 3. If dH < 0 energetically favourable -> Accept trial and change the type
        if (SlipState_Vector.at(NewSlipNumber) == 1) {
            SlipState_Vector.at(NewSlipNumber) = 0;
        } //if
     // 4. Else if dH > 0 consider the acceptance probability
        else if (SlipState_Vector.at(NewSlipNumber) == 0) {
    // Model variables
            double Snt = 0.0, s2 = 0.0;
            vector<vector<double>> sik{{0,0,0},{0,0,0},{0,0,0}};

//            cout << NewSlipNumber << "\t" << slip_vector.at(NewSlipNumber) << endl;
            //exit(457);

            for (int i = 0; i < 3; ++i)
                for (int j = 0; j < 3; ++j) {
                    sik[i][j] = 0.5 * slip_vector.at(NewSlipNumber) * (norms_vector.at(NewSlipNumber)[i] * tang_vector.at(NewSlipNumber)[j] +
                                                norms_vector.at(NewSlipNumber)[j] * tang_vector.at(NewSlipNumber)[i]);
                    Snt += stress_tensor[i][j] * sik[i][j];
                     s2 += 0.5 * pow(slip_vector.at(NewSlipNumber),2) * (norms_vector.at(NewSlipNumber)[i] * tang_vector.at(NewSlipNumber)[j]*norms_vector.at(NewSlipNumber)[i] * tang_vector.at(NewSlipNumber)[j] +
                            norms_vector.at(NewSlipNumber)[i] * tang_vector.at(NewSlipNumber)[j]*norms_vector.at(NewSlipNumber)[j] * tang_vector.at(NewSlipNumber)[i]);
                }

            /// New ACCEPTANCE PROBABILITY
            double P_ac = exp(- (alpha * s2 - Snt - lambda * slip_vector.at(NewSlipNumber))/(Rc * Temperature));
            if (P_ac > 1) P_ac = 1.0;
                //cout << "P_ac =\t" << alpha * s2 - Snt - lambda * slip_vector.at(NewSlipNumber) << "\trandom number\t" << (alpha * s2 - Snt - lambda * slip_vector.at(NewSlipNumber))/(Rc * Temperature) << endl;

            double rv = (rand() / (RAND_MAX + 1.0)); // Generate random value in the range [0,1]
                if (rv <= P_ac) {
                //     if (P_ac > 0) cout << "P_ac =\t" << (alpha*pow(s,2.0) - ShearStress * slip_vector.at(NewSlipNumber) - lambda * slip_vector.at(NewSlipNumber))/(Rc * Temperature) << "\trandom number\t" << rv << endl;
                    SlipState_Vector.at(NewSlipNumber) = 1;
                } else continue;
        } // end of  else if (SlipState_Vector.at(NewSlipNumber) == 0)

    } // for loop (i < iteration_number)

    return SlipState_Vector;
} // end of Metropolis

vector<vector<double>> lt_vector(vector<vector<double>> &stress_tensor, vector<vector<double>> norms_vector) {
    vector<vector<double>> tv, tnv, ttv, lt; // traction vector tv and its normal (tnv) and tangent (ttv) components + tangential vector to the slip plane lt

//for (auto kl : stress_tensor.at(1))    cout << kl << endl;
    for (unsigned int fnumb = 0; fnumb < norms_vector.size(); ++fnumb) { // loop over all the slip elements in the complex

        double tv0 = inner_product(stress_tensor.at(0).begin(), stress_tensor.at(0).end(),
                                   norms_vector.at(fnumb).begin(), 0);
        double tv1 = inner_product(stress_tensor.at(1).begin(), stress_tensor.at(1).end(),
                                   norms_vector.at(fnumb).begin(), 0);
        double tv2 = inner_product(stress_tensor.at(2).begin(), stress_tensor.at(2).end(),
                                   norms_vector.at(fnumb).begin(), 0);
        tv.push_back({tv0, tv1, tv2});

        double tnv0 = inner_product(tv.at(fnumb).begin(), tv.at(fnumb).end(), norms_vector.at(fnumb).begin(), 0) *
                      norms_vector[fnumb][0];
        double tnv1 = inner_product(tv.at(fnumb).begin(), tv.at(fnumb).end(), norms_vector.at(fnumb).begin(), 0) *
                      norms_vector[fnumb][1];
        double tnv2 = inner_product(tv.at(fnumb).begin(), tv.at(fnumb).end(), norms_vector.at(fnumb).begin(), 0) *
                      norms_vector[fnumb][2];
        tnv.push_back({tnv0, tnv1, tnv2});
        double ttv0 = tv0 - tnv0, ttv1 = tv1 - tnv1, ttv2 = tv2 - tnv2;
        ttv.push_back({ttv0, ttv1, ttv2});

        double  lt0 = 0.0, lt1 = 0.0, lt2 = 0.0;
        if (inner_product(ttv.at(fnumb).begin(), ttv.at(fnumb).end(), ttv.at(fnumb).begin(), 0.0L) != 0) {
            lt0 =
                    ttv0 / sqrt(inner_product(ttv.at(fnumb).begin(), ttv.at(fnumb).end(), ttv.at(fnumb).begin(), 0.0L));
             lt1 =
                    ttv1 / sqrt(inner_product(ttv.at(fnumb).begin(), ttv.at(fnumb).end(), ttv.at(fnumb).begin(), 0.0L));
             lt2 =
                    ttv2 / sqrt(inner_product(ttv.at(fnumb).begin(), ttv.at(fnumb).end(), ttv.at(fnumb).begin(), 0.0L));
        }
        lt.push_back({lt0, lt1, lt2});

    } // end of for(fnumb < norms_vector.size())

    return lt;
} /// end of lt_vector() function
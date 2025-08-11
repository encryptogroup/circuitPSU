#include <vector>
#include "cryptoTools/Circuit/BetaCircuit.h"
#include <tuple>
#include <stdio.h>   
#include <stdlib.h> 

#include <fstream>

namespace osuCrypto
{
    class ShareTranslationCircuit
    {


        public:
        // returns out 0, out 1
        static void AddSwitch(oc::BetaWire a, oc::BetaWire b,oc::BetaWire control,oc::BetaCircuit* circ, oc::BetaWire* out0,oc::BetaWire* out1)
        {
            oc::BetaWire ab0;
            oc::BetaWire abp0;
            oc::BetaWire o0;
            oc::BetaWire o1;
            circ->addTempWire(ab0);
            circ->addTempWire(abp0);
            circ->addTempWire(o0);
            circ->addTempWire(o1);
            //std::cout << circ->mWireCount << " " << a << " " << control << " " << a0 << "\n";
            
            circ->addGate(a, b, oc::GateType::Xor, ab0);
            circ->addGate(ab0, control, oc::GateType::And, abp0);
            circ->addGate(abp0, a, oc::GateType::Xor, o0);
            circ->addGate(abp0, b, oc::GateType::Xor, o1);
            *out0 = o0;
            *out1 = o1;
        }

        
        static int getProgNumber(int n)
        {
            if (n == 1) return 0;
            int k = n / 2;
            int k2 = (n%2 == 0)? (n/2) -1 : (n/2);
            return getProgNumber(k)+getProgNumber(n-k) + k+k2;
        }


class WaksmanRepresentation
{
        // This gives index of bit in prog vector
    public:

        
	    class TodoList {
		uint32_t* nextu;
		uint32_t* prevu;
		uint32_t n;

	    public:
		    void remove(uint32_t x) {
			    nextu[prevu[x]] = nextu[x];
			    prevu[nextu[x]] = prevu[x];
		    }

		    uint32_t next() {
			    uint32_t ret = nextu[n];
			    if (ret == n)
			    	return -1;
			    else
			    	return ret;
		    }

		    TodoList(uint32_t num) {
			    n = num;
			    nextu = (uint32_t*) malloc(sizeof(uint32_t) * (n + 1)); //new uint32_t[n+1];
			    prevu = (uint32_t*) malloc(sizeof(uint32_t) * (n + 1)); //new uint32_t[n+1];
			    for (uint32_t i = 0; i < n + 1; i++) {
			    	nextu[i] = (i + 1) % (n + 1);
			    	prevu[i] = (i + n) % (n + 1);
			    }
		    }
            ~TodoList()
            {
                free(nextu);
                free(prevu);
            }
	    };

        // for in switches
        std::vector<int> s1;

        // for out switches
        std::vector<int> s2;
        oc::BitVector* prog;
        int sizeB2;
        int n;
        WaksmanRepresentation* b1;
        WaksmanRepresentation* b2;
        TodoList* Todo;
        // local number of prog bits
        int numberProgBits = 0;

        WaksmanRepresentation(int inputNum,int prevProgBits, oc::BitVector* bits)
        {

            n = inputNum;
            
            numberProgBits = prevProgBits;
            prog = bits;
	        Todo = new TodoList(n / 2);
            if(n != 1)
            {
                
                s1.resize(n/2);
                for(int i = 0;i < (n/2);i++)
                {
                    s1[i] = numberProgBits++;
                }

                b1 = new WaksmanRepresentation(n/2,numberProgBits,prog);
                b2 = new WaksmanRepresentation(n - (n/2),b1->numberProgBits,prog);
                sizeB2 = (n%2 == 0) ? n/2 - 1 : n / 2;
                numberProgBits = b2->numberProgBits;
                s2.resize(sizeB2);

                for(int i = 0;i<sizeB2;i++)
                {
                    s2[i] = numberProgBits++;
                }
            }
        }


        // RECURSIVE PROGRAMING ALGORITHM: WRITES PROGRAMING INTO WAKSMAN REPRESENTATION

    void program_rec(uint32_t in, uint32_t block, uint32_t* p1, uint32_t* p2, uint32_t* rows, uint32_t* cols) {
	        uint32_t out = rows[in];
            

            //std::cout << "Working on " << in << " and " << block << "\n";
	        if ((in ^ 1) < n && rows[in ^ 1] != UINT_MAX) {
                (*(prog))[s1[in / 2]] = (block == 0) != (in % 2 == 0);
		        Todo->remove(in / 2);
	        }

	        if (block == 1) {
		        p2[in / 2] = out / 2;
		        if (out / 2 < sizeB2) {
                    (*(prog))[s2[out/2]] = out % 2 == 0;
		        }
	        } else { // block==0
		        p1[in / 2] = out / 2;
		        if (out / 2 < sizeB2) {
                    (*(prog))[s2[out/2]] = out % 2 == 1;
		        }
	        }
	        rows[in] = UINT_MAX;
	        cols[out] = UINT_MAX;

	        uint32_t newout = out ^ 1;
	        if (newout < n && cols[newout] != UINT_MAX) {
		        uint32_t newin = cols[newout];
		        cols[newout] = UINT_MAX;

		        program_rec(newin, block ^ 1, p1, p2, rows, cols);
	        }

	        if ((in ^ 1) < n && rows[in ^ 1] != UINT_MAX) {
		        program_rec(in ^ 1, block ^ 1, p1, p2, rows, cols);
	        }
    }

};

    


static void program(uint32_t* perm,WaksmanRepresentation* waksmanRepresentation) {
    int nInput = waksmanRepresentation->n;
	if (nInput == 1)
		return;

	uint32_t* rows = perm;

	uint32_t* cols = (uint32_t*) malloc(sizeof(uint32_t) * nInput); //new uint32_t[v];
	for (uint32_t i = 0; i < nInput; i++) {
		uint32_t x = perm[i];
		cols[x] = i;
	}

	// programs for sub-blocks
	uint32_t* p1 = (uint32_t*) malloc(sizeof(uint32_t) * (nInput / 2)); //new uint32_t[u / 2];
	uint32_t* p2 = (uint32_t*) malloc(sizeof(uint32_t) * (nInput - (nInput / 2))); //new uint32_t[u - (u / 2)];

	if (nInput % 2 == 1) { // case c+d and b+d
		waksmanRepresentation->program_rec(nInput - 1, 1, p1, p2, rows, cols);
		if (cols[nInput - 1] != UINT_MAX)
			waksmanRepresentation->program_rec(cols[nInput - 1], 1, p1, p2, rows, cols);
	}

	if (nInput % 2 == 0) { // case a
		if (cols[nInput - 1] != UINT_MAX)
			waksmanRepresentation->program_rec(cols[nInput - 1], 1, p1, p2, rows, cols);
		if (cols[nInput - 2] != UINT_MAX)
			waksmanRepresentation->program_rec(cols[nInput - 2], 0, p1, p2, rows, cols);
	}

	for (uint32_t n = waksmanRepresentation->Todo->next(); n != UINT_MAX; n = waksmanRepresentation->Todo->next()) {
		waksmanRepresentation->program_rec(2 * n, 0, p1, p2, rows, cols);
	}

	    // program sub-blocks
	    program(p1,waksmanRepresentation->b1);
        free(p1);
	    program(p2,waksmanRepresentation->b2);
        free(p2);
        waksmanRepresentation->s1.clear();
        waksmanRepresentation->s2.clear();
        free(cols);
        free(waksmanRepresentation->Todo);
    }








        static oc::BitVector GenerateProgramming(std::vector<oc::u32>* pi)
        {
            auto bits = oc::BitVector(getProgNumber(pi->size()));
            WaksmanRepresentation* waksmanRepresentation = new WaksmanRepresentation(pi->size(),0,&bits);

            // This just shifts the datastructure to reduce memory demand
            uint32_t* pimove = (uint32_t*) malloc(pi->size()*sizeof(uint32_t));
            for(int i = 0;i<pi->size();i++)
            {
                pimove[i] = (uint32_t) (*pi)[i];
            }

            program(pimove,waksmanRepresentation);
            return bits;
        }






        static int GenerateSubNetwork(oc::BetaWire* inputs,oc::BetaBundle* controlBits,oc::BetaCircuit* circ,int n,int wireOffset)
        {

            if(n == 1)
            {
                return wireOffset;
            }
            
            int k = n/2;
                
            oc::BetaWire* inputsTop = (oc::BetaWire*) malloc(k*sizeof(oc::BetaWire));
            oc::BetaWire* inputsBot = (oc::BetaWire*) malloc((n-k)*sizeof(oc::BetaWire));
            for(int i = 0;i<(k);i++)
            {
                  AddSwitch(inputs[2*i],inputs[2*i+1],controlBits->mWires[wireOffset+i],circ,&(inputs[2*i]),&(inputs[2*i+1]));
                  inputsTop[i] = inputs[2*i];
                  inputsBot[i] = inputs[2*i+1];
            }
            if(n%2==1)
            {
                inputsBot[n-k-1] = inputs[n-1];
            }

            int wo = wireOffset + k;
            int new_wireOffset = 0;
            //std::cout << "Test2 " << circ->mWireCount << "\n";
            //throw "LOL";
            //std::cout << "Test " << o << "\n";
            new_wireOffset = GenerateSubNetwork(inputsTop,controlBits,circ,k,wo);
            //std::cout << "Test after " << o << "\n";
            new_wireOffset = GenerateSubNetwork(inputsBot,controlBits,circ,n-k,new_wireOffset);


            
            int k2 = (n%2 == 0)? (n/2) - 1 : (n/2);
            for(int i = 0;i<(k2);i++)
            {
                AddSwitch(inputsTop[i],inputsBot[i],controlBits->mWires[new_wireOffset+i],circ,&(inputs[2*i]),&(inputs[2*i+1]));
            }

            inputs[n-1] = inputsBot[n-k-1];
            if(n % 2 == 0)
            {
                inputs[n-2] = inputsTop[n/2-1];
            }
            return new_wireOffset + (k2);
        }



        static void GenerateCircuit(long n,oc::BetaCircuit* circ)
        {


	        std::ifstream in;
            std::string filename = "./circuit_"+std::to_string(n)+".bin";
	        in.open(filename, std::ios::in | std::ios::binary);
            if(in.is_open() == false)
            {
                // input bundle first n bit: a second n bit: b
                oc::BetaBundle inputs_permutation(2*n);
                circ->addInputBundle(inputs_permutation);
                int prognum = getProgNumber(n);
                oc::BetaBundle inputs_programming(prognum);
                circ->addInputBundle(inputs_programming);
            
                oc::BetaBundle output(n);
                for(int i = 0;i<n;i++)
                {
                
                   oc::BetaWire o;
                   circ->addTempWire(o);
                    output.mWires[i] = o;
                }
                circ->addOutputBundle(output);

                std::cout << "Generate Network with "<< prognum << " Bits\n";
                std::vector<oc::BetaWire> inputs_a(n);
                std::vector<oc::BetaWire> inputs_b(n);
                for(int i = 0;i<n;i++)
                {
                    inputs_a[i] = inputs_permutation.mWires[i];
                    inputs_b[i] = inputs_permutation.mWires[i+n];
                }
                GenerateSubNetwork(inputs_a.data(),&inputs_programming,circ,n,0);

                // inputs_a was now modified inplace to have the wires out of the permutation
            
                for(int i = 0;i<n;i++)
                {
                    circ->addGate(inputs_a[i], inputs_b[i], oc::GateType::Xor, output.mWires[i]);
                }
            
                circ->levelByAndDepth(BetaCircuit::LevelizeType::Reorder);
            
                in.close();
	            std::ofstream out;
	            out.open(filename, std::ios::out | std::ios::trunc | std::ios::binary);
                circ->writeBin(out);
                out.close();
                std::cout << "Circuit written to disk\n";
            }else{
                std::cout << "Reading circuit from disk\n";
                circ->readBin(in);
                in.close();
            }

        }
        
    };
}
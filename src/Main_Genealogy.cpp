
#include "Main.h"
//#include "Analytics.h"
#include <iostream>
#include <fstream>
#include <sys/stat.h>
#include <algorithm>
#include "SphereWorld.h"

static int critter_width = 200;
static int critter_height = 46;
static int segment_width = 20;
static int segment_height = 20;

const int TOP_N_CRITTERS = 8;
const int MIN_CRITTER_WIDTH = 100;

using namespace std;

inline bool skipGenome(string genome) {
    // single-cell sleep critters are a mutation that aren't viable (will never reproduce) and yet are over-represented in
    // the tree of life simply because they take a long time to die. Ignore them.
    return (genome.size() == 1 && (genome[0] & eInstructionMask) == eInstructionSleep);
}

class GenomeBranch {
public:
    GenomeBranch() {parent = NULL;}
    ~GenomeBranch() {
        for (vector<GenomeBranch*>::iterator i = descendants.begin(); i != descendants.end(); i++) {
            delete *i;
        }
    }
    
    void addDescendant(GenomeBranch *pDescendant) {
        // prevent circular descendancy
        GenomeBranch * pTest = this;
        while (pTest != NULL) {
            if (pTest->genome == pDescendant->genome)
                return;
            pTest = pTest->parent;
        }
        descendants.push_back(pDescendant);
    }
	
	GenomeBranch * getDescendant(string genome) {
		for (std::vector<GenomeBranch*>::iterator i = descendants.begin(); i != descendants.end(); i++) {
			if ((*i)->genome == genome) {
				return *i;
			}
		}
		return NULL;
	}
	
	void getEndNodes(vector<GenomeBranch*> & result) {
		
		if (descendants.empty()) {
			result.push_back(this);
		}
		else {
			for (int i = 0; i < descendants.size(); i++) {
				descendants[i]->getEndNodes(result);
			}
		}
	}
	
	void removeDescendant(GenomeBranch *child) {
		for (std::vector<GenomeBranch*>::iterator i = descendants.begin(); i != descendants.end(); i++) {
			if ((*i) == child) {
				descendants.erase(i);
				break;
			}
		}
	}

	const std::vector<GenomeBranch*> & getDescendants() { return descendants; }
    
    std::string genome;
    long turnAppeared;
    GenomeBranch * parent;

private:
    std::vector<GenomeBranch*> descendants;
};


int getMaxGenomeLength(GenomeBranch *pGenomeBranch, int curMax = 0);
int getMaxGenomeLength(GenomeBranch *pGenomeBranch, int curMax)
{
    if (pGenomeBranch->genome.length() > curMax)
        curMax = pGenomeBranch->genome.length();
    
    for (vector<GenomeBranch*>::const_iterator i = pGenomeBranch->getDescendants().begin(); i != pGenomeBranch->getDescendants().end(); i++) {
        if (! skipGenome((*i)->genome))
            curMax = getMaxGenomeLength(*i, curMax);
    }
    return curMax;
}

bool compareDecendantsFunc(GenomeBranch * p1, GenomeBranch * p2);
bool compareDecendantsFunc(GenomeBranch * p1, GenomeBranch * p2)
{
    return p2->getDescendants().size() > p1->getDescendants().size();
}

inline string toHTMLGenome(const char *pGenome) {
    string result;
    
    while (*pGenome) {
        char ch = *pGenome++;
        result += 'a' + (ch & eInstructionMask) - eInstructionPhotosynthesize;
		
		eSegmentExecutionType execType = Genome::getExecType(ch);
		switch (execType) {
			case eAlways:	result += 'A'; break;
			case eIf:		result += 'T'; break;
			case eNotIf:	result += 'F'; break;
				
			default:
				result += "*error*";
		}
    }
    return result;
}
inline string toHTMLGenome(const string & str) { return toHTMLGenome(str.c_str()); }

int printTree(GenomeBranch *pGenomeBranch, set<string> & livingGenomes, map<string,int> & genomeToPopulation, set<string> &addedGenomes,
               stringstream & f, int generationLevel = 0, int siblingLevel = 0, const char * parentGenome = NULL, int minPopulation = 0);

int printTree(GenomeBranch *pGenomeBranch, set<string> & livingGenomes, map<string,int> & genomeToPopulation, set<string> &addedGenomes,
               stringstream & f, int generationLevel, int siblingLevel, const char *parentGenome, int minPopulation)
{
	int result = 1;
	
    int x = siblingLevel * critter_width * 1.25;
    int y = generationLevel * (critter_height + 50);
    
    string &genome = pGenomeBranch->genome;
    bool alive = false;
    
    f << "<div id='critter_" << toHTMLGenome(genome) << "' class='critter";
    if (livingGenomes.find(genome) != livingGenomes.end()) {
        f << " alive";
        alive = true;
    }
    
    f << "' style='left:" << x << "px;top:" << y << "px' data-generation=" << generationLevel
        << " data-turn-appeared=" << pGenomeBranch->turnAppeared << " data-genome='" << toHTMLGenome(genome) << "'";
    
    if (parentGenome != NULL) {
        f << " data-parent-genome='" << toHTMLGenome(parentGenome) << "'";
    }
    f << ">" << '\n';
    
    int sx = (critter_width - genome.length() * segment_width) / 2;
    int sy = 4;

    for (int i = 0; i < genome.size(); i++) {

        for (int pass = 0; pass < 2; pass++) {
            char ch = genome[i];
            if (pass == 0) {
                ch = 'a' + ((ch & eInstructionMask) - eInstructionPhotosynthesize);
            }
            else {
				eSegmentExecutionType execType = Genome::getExecType(ch);
				switch (execType) {
					default:
					case eAlways:	ch = 0; break;
					case eIf:		ch = 'Y'; break;
					case eNotIf:	ch = 'N'; break;
				}
            }
            if (ch != 0) {
                f << "<div class='segment segment_";
                if (ch == 'Y' || (ch == 'N'))
                    f << '_';
                f << ch;
                f << "' style='left:" << sx << "px;top:" << sy << "px'></div>";
            }
        }
        sx += segment_width;
    }
    
    if (alive) {
        f << "<p class='population'>Population: " << genomeToPopulation[genome] << "</p>";
    }
    else {
        f << "<p class='population'><i>EXTINCT</i></p>";
    }
    f << "</div>" << '\n';
	

	vector<GenomeBranch*> children;
    
    for (vector<GenomeBranch*>::const_iterator i = pGenomeBranch->getDescendants().begin(); i != pGenomeBranch->getDescendants().end(); i++) {
        if (addedGenomes.find((*i)->genome) != addedGenomes.end()) {
            continue;
        }
        addedGenomes.insert((*i)->genome);
        
        if (! skipGenome((*i)->genome)) {
            children.push_back(*i);
        }
    }
    sort(children.begin(), children.end(), compareDecendantsFunc);
    
    for (vector<GenomeBranch*>::iterator i = children.begin(); i != children.end(); i++) {
		if ((*i)->getDescendants().size() == 0) {
			if (genomeToPopulation[(*i)->genome] < minPopulation)
				return siblingLevel;
		}
        int maxSibs = printTree(*i, livingGenomes, genomeToPopulation, addedGenomes, f, generationLevel + 1, siblingLevel++, genome.c_str(), minPopulation);
		if (maxSibs > siblingLevel)
			siblingLevel = maxSibs;
    }
	return siblingLevel;
}

std::string replaceAll(std::string str, const std::string& from, const std::string& to);
std::string replaceAll(std::string str, const std::string& from, const std::string& to) {
    size_t start_pos = 0;
    while((start_pos = str.find(from, start_pos)) != std::string::npos) {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length(); // Handles case where 'to' is a substring of 'from'
    }
    return str;
}

inline std::string toString(int x) {
    char buffer[200];
    sprintf(buffer, "%d", x);
    return buffer;
}

inline std::string toString(float x) {
    char buffer[200];
    sprintf(buffer, "%f", x);
    return buffer;
}

static void addGenomeToTree(SphereWorld & world, string genome, map<string,GenomeBranch*> & genomesToNode, GenomeBranch * pChild = NULL);
static void addGenomeToTree(SphereWorld & world, string genome, map<string,GenomeBranch*> & genomesToNode, GenomeBranch * pChild)
{
    GenomeBranch * pDescendant = genomesToNode[genome];

	if (pDescendant == NULL) {
		
		pDescendant = new GenomeBranch();
		pDescendant->genome = genome;
		genomesToNode[genome] = pDescendant;

		const char *pGenome = genome.c_str();
		pDescendant->turnAppeared = world.getFirstTurn(pGenome);
		
		string parentGenome = world.getParentGenome(pGenome);
		if (parentGenome.length() > 0) {
			addGenomeToTree(world, parentGenome, genomesToNode, pDescendant);
		}
	}

	if (pChild != NULL) {
        pDescendant->addDescendant(pChild);
        pChild->parent = pDescendant;
    }
    
}
GenomeBranch * getTree(SphereWorld & world, int maxSpecies = TOP_N_CRITTERS);
GenomeBranch * getTree(SphereWorld & world, int maxSpecies) {
    
    map<string,GenomeBranch*> genomesToNode;
    
    vector<pair<string,int> > & topSpecies = world.getTopSpecies();
    if (maxSpecies <= 0) {
        maxSpecies = topSpecies.size();
    }
    
	string topGenome;
	topGenome += eInstructionPhotosynthesize;

	
	int numEndGenomes = 0;
	
	int minPopulation = 0;
	
    for (int i = 0; i < topSpecies.size() && i < maxSpecies; i++) {
        string genome = topSpecies[i].first;
        
        addGenomeToTree(world, genome, genomesToNode);
		
		/*
		if (! world.hasChildGenomes(genome.c_str()))
			++numEndGenomes;
		
		if (numEndGenomes >= maxSpecies) {
			break;
		}
		 */
	}

    GenomeBranch * pTop = genomesToNode[topGenome];
	if (pTop == NULL)
		return NULL;
	
	/*
	vector<GenomeBranch*> endNodes;
	pTop->getEndNodes(endNodes);
	while (endNodes.size() > maxSpecies) {
		GenomeBranch * removeNode = endNodes[endNodes.size()-1];
		
		while (removeNode->parent->getDescendants().size() == 1) {
			GenomeBranch * newParent = removeNode->parent;
			newParent->removeDescendant(removeNode);
			removeNode = newParent;
		}
		
		endNodes.erase(--endNodes.end());
	}
	*/

	pTop->turnAppeared = 0;


	/*
	// verify that the top species are in the tree
	for (int i = 0; (i < topSpecies.size()) && (i < maxSpecies); i++) {
		string genome = topSpecies[i].first;
		
		printf("looking for genome: %s\nParents: ", toHTMLGenome(genome).c_str());
		deque<string> parents;
		while (true) {
			genome = world.getParentGenome(genome.c_str());
			if (genome.length() == 0)
				break;
			parents.push_front(genome);
			printf("%s ", toHTMLGenome(genome).c_str());
		}
		printf("\n\nNow searching from top down\n");
		
		
		GenomeBranch * node = pTop;
		for (int j = 0; j < parents.size(); j++) {
			if (j == 0) {
				assert(node->genome == parents[j]);
				continue;
			}
			node = node->getDescendant(parents[j]);
			if (node != NULL) {
				printf("Node %s has descendant %s\n", toHTMLGenome(node->genome).c_str(), toHTMLGenome(parents[j]).c_str());
			}

			assert(node != NULL);
		}
		genome = topSpecies[i].first;
		assert(node->genome == genome || node->getDescendant(genome) != NULL);
	}
	 */
	
	return pTop;
}

void Main::handleGenealogy()
{
    LockWorldMutex m;
    GenomeBranch * pTree = getTree(world);
	if (pTree == NULL)
		return;
    set<string> livingGenomes;
    map<string,int> genomeToPopulation;
    
    std::vector<std::pair<std::string,int> > & topSpecies = world.getTopSpecies();

	int minPopulation = 0;
	for (int i = 0; i < topSpecies.size(); i++) {
        livingGenomes.insert(topSpecies[i].first);
        genomeToPopulation[topSpecies[i].first] = topSpecies[i].second;

		if (i < TOP_N_CRITTERS) {
			minPopulation = topSpecies[i].second;
		}
    }

    string genealogyPath = getenv("HOME");
    string from = "/private";
    string to = "";
    
    size_t start_pos = genealogyPath.rfind(from.c_str());
    if (start_pos != std::string::npos) {
        genealogyPath.replace(start_pos, from.length(), to);
    }
    genealogyPath += "/Documents";

	Stream* inStream = FileSystem::open("res/genealogyTemplate.html", FileSystem::READ);
	size_t templateLength = inStream->length();
	char *buffer = new char[templateLength+1];
	inStream->read(buffer,1,templateLength);
	buffer[templateLength] = 0;
	inStream->close();
	string contents(buffer);
	delete[] buffer;

	genealogyPath += "/genealogy.html";
    
    std::stringstream treeStream;
    critter_width = segment_width * (getMaxGenomeLength(pTree)+ 1);
    if (critter_width < MIN_CRITTER_WIDTH)
        critter_width = MIN_CRITTER_WIDTH;

    set<string> addedGenomes;
	printTree(pTree, livingGenomes, genomeToPopulation, addedGenomes, treeStream, 0, 0, NULL, minPopulation);
    
    contents = replaceAll(contents, string("[[generation_height]]"), toString(critter_height * 1.4f));
    contents = replaceAll(contents, string("[[turn_height_multiplier]]"), toString(.1f));
    contents = replaceAll(contents, string("[[contents]]"), treeStream.str());
    contents = replaceAll(contents, string("[[critter_width]]"), toString(critter_width));
    contents = replaceAll(contents, string("[[critter_height]]"), toString(critter_height));
    contents = replaceAll(contents, string("[[segment_width]]"), toString(segment_width));
    contents = replaceAll(contents, string("[[segment_height]]"), toString(segment_height));

    string resPath = string("file://") + FileSystem::getResourcePath() + "res";
    contents = replaceAll(contents, string("[[res_path]]"), resPath);
    
	Stream* outStream = FileSystem::open(genealogyPath.c_str(), FileSystem::WRITE);
	outStream->write(contents.c_str(), 1, contents.length());
	outStream->close();
    delete pTree;    
    
    openURL(genealogyPath.c_str(), false);
}
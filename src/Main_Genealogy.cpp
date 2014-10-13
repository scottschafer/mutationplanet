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

const int MIN_CRITTER_WIDTH = 100;

using namespace std;

inline bool skipGenome(string genome) {
    // single-cell sleep critters are a mutation that aren't viable (will never reproduce) and yet are over-represented in
    // the tree of life simply because they take a long time to die. Ignore them.
    return (genome.size() == 1 && (genome[0] & eInstructionMask) == eInstructionSleep);
}

class ParentGenome {
public:
    ParentGenome() {parent = NULL;}
    ~ParentGenome() {
        for (vector<ParentGenome*>::iterator i = descendants.begin(); i != descendants.end(); i++) {
            delete *i;
        }
    }
    
    void addDescendant(ParentGenome *pDescendant) {
        // prevent circular descendancy
        ParentGenome * pTest = this;
        while (pTest != NULL) {
            if (pTest->genome == pDescendant->genome)
                return;
            pTest = pTest->parent;
        }
        descendants.push_back(pDescendant);
    }
    const std::vector<ParentGenome*> & getDescendants() { return descendants; }

    
    std::string genome;
    long turnAppeared;
    ParentGenome * parent;

private:
    std::vector<ParentGenome*> descendants;
};


int getMaxGenomeLength(ParentGenome *pParentGenome, int curMax = 0);
int getMaxGenomeLength(ParentGenome *pParentGenome, int curMax)
{
    if (pParentGenome->genome.length() > curMax)
        curMax = pParentGenome->genome.length();
    
    for (vector<ParentGenome*>::const_iterator i = pParentGenome->getDescendants().begin(); i != pParentGenome->getDescendants().end(); i++) {
        if (! skipGenome((*i)->genome))
            curMax = getMaxGenomeLength(*i, curMax);
    }
    return curMax;
}

bool compareDecendantsFunc(ParentGenome * p1, ParentGenome * p2);
bool compareDecendantsFunc(ParentGenome * p1, ParentGenome * p2)
{
    return p2->getDescendants().size() > p1->getDescendants().size();
}

inline string toHTMLGenome(const char *pGenome) {
    string result;
    
    while (*pGenome) {
        char ch = *pGenome++;
        result += 'a' + (ch & eInstructionMask);
        if (ch & eAlways)
            result += 'A';
        else
            if (ch & eIf)
                result += 'T';
            else
                result += 'F';
    }
    return result;
}
inline string toHTMLGenome(const string & str) { return toHTMLGenome(str.c_str()); }

void printTree(ParentGenome *pParentGenome, set<string> & livingGenomes, map<string,int> & genomeToPopulation, set<string> &addedGenomes,
               stringstream & f, int generationLevel = 0, int siblingLevel = 0, const char * parentGenome = NULL);

void printTree(ParentGenome *pParentGenome, set<string> & livingGenomes, map<string,int> & genomeToPopulation, set<string> &addedGenomes,
               stringstream & f, int generationLevel, int siblingLevel, const char *parentGenome)
{
    int x = siblingLevel * critter_width * 1.25;
    int y = generationLevel * (critter_height + 50);
    
    string &genome = pParentGenome->genome;
    bool alive = false;
    
    f << "<div id='critter_" << toHTMLGenome(genome) << "' class='critter";
    if (livingGenomes.find(genome) != livingGenomes.end()) {
        f << " alive";
        alive = true;
    }
    
    f << "' style='left:" << x << "px;top:" << y << "px' data-generation=" << generationLevel
        << " data-turn-appeared=" << pParentGenome->turnAppeared << " data-genome='" << toHTMLGenome(genome) << "'";
    
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
                if (ch & eAlways)
                    ch = 0;
                else if (ch & eIf)
                    ch = 'Y';
                else
                    ch = 'N';
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
    
    vector<ParentGenome*> children;
    
    for (vector<ParentGenome*>::const_iterator i = pParentGenome->getDescendants().begin(); i != pParentGenome->getDescendants().end(); i++) {
        if (addedGenomes.find((*i)->genome) != addedGenomes.end()) {
            continue;
        }
        addedGenomes.insert((*i)->genome);
        
        if (! skipGenome((*i)->genome)) {
            children.push_back(*i);
        }
    }
    sort(children.begin(), children.end(), compareDecendantsFunc);
    
    for (vector<ParentGenome*>::iterator i = children.begin(); i != children.end(); i++) {
        printTree(*i, livingGenomes, genomeToPopulation, addedGenomes, f, generationLevel + 1, siblingLevel++, genome.c_str());
    }
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

static void addGenomeToTree(SphereWorld & world, string genome, map<string,ParentGenome*> & genomesToAncestors, set<string>& addedGenomes, ParentGenome * pChild = NULL);
static void addGenomeToTree(SphereWorld & world, string genome, map<string,ParentGenome*> & genomesToAncestors, set<string>& addedGenomes, ParentGenome * pChild)
{
    ParentGenome * pDescendant = genomesToAncestors[genome];

    if (addedGenomes.find(genome) == addedGenomes.end()) {
        addedGenomes.insert(genome);
        
        if (pDescendant == NULL) {
            
            pDescendant = new ParentGenome();
            pDescendant->genome = genome;
            genomesToAncestors[genome] = pDescendant;

            const char *pGenome = genome.c_str();
            pDescendant->turnAppeared = world.getFirstTurn(pGenome);
            
            string parentGenome = world.getParentGenome(pGenome);
            if (parentGenome.length() > 0)
                addGenomeToTree(world, parentGenome, genomesToAncestors, addedGenomes, pDescendant);
        }
        
    }
    if (pChild != NULL) {
        pDescendant->addDescendant(pChild);
        pChild->parent = pDescendant;
    }
    
}

ParentGenome * getTree(SphereWorld & world, int maxSpecies = 5);
ParentGenome * getTree(SphereWorld & world, int maxSpecies) {
    
    map<string,ParentGenome*> genomesToAncestors;
    
    vector<pair<string,int> > & topSpecies = world.getTopSpecies();
    if (maxSpecies <= 0) {
        maxSpecies = topSpecies.size();
    }
    
    set<string> alreadyAdded;
    
    for (int i = 0; (i < topSpecies.size()) && (i < maxSpecies); i++) {
        string genome = topSpecies[i].first;
        
        addGenomeToTree(world, genome, genomesToAncestors, alreadyAdded);
    }
    
    string top;
    top += eInstructionPhotosynthesize;
    ParentGenome * pTop = genomesToAncestors[top];
    pTop->turnAppeared = 0;
    return pTop;
}

/**
 * sss: my attempt to clean up the childToParentGenomes map so it doesn't get outrageously large.
 */
void addTreeToSet(ParentGenome * genome, set<string> & genomes);
void addTreeToSet(ParentGenome * parent, set<string> & genomes) {
    genomes.insert(parent->genome);
    for (vector<ParentGenome*>::const_iterator i = parent->getDescendants().begin(); i != parent->getDescendants().end(); i++) {
        addTreeToSet(*i, genomes);
    }
}

void prune(SphereWorld & world, std::map<std::string, std::string> & childToParentGenomes);
void prune(SphereWorld & world, std::map<std::string, std::string> & childToParentGenomes){

    return;
    set<string> & livingGenomes = world.getLivingGenomes();
    set<string> unprunableGenomes;
    string photoSynthesize;
    photoSynthesize +=eInstructionPhotosynthesize;
    unprunableGenomes.insert(photoSynthesize);
    
    for (set<string>::iterator i = livingGenomes.begin(); i != livingGenomes.end(); i++) {
        string genome = *i;
        while(unprunableGenomes.find(genome) == unprunableGenomes.end()) {
            unprunableGenomes.insert(genome);
            genome = childToParentGenomes[genome];
            if (genome.length() == 0) {
                break;
            }
        }
    }
    
    std::list< std::map<string,string>::iterator > iteratorList;
    for (map<string,string>::iterator i = childToParentGenomes.begin(); i != childToParentGenomes.end(); i++) {
        if (unprunableGenomes.find(i->first) == unprunableGenomes.end() && unprunableGenomes.find(i->second) == unprunableGenomes.end()) {
            iteratorList.push_back(i);
        }
    }
    
    for(map<string,string>::iterator i : iteratorList){
        childToParentGenomes.erase(i);
    }
}

void Main::handleGenealogy()
{
    LockWorldMutex m;
    ParentGenome * pTree = getTree(world);
    set<string> livingGenomes;
    map<string,int> genomeToPopulation;
    
    std::vector<std::pair<std::string,int> > & topSpecies = world.getTopSpecies();
    for (int i = 0; i < topSpecies.size(); i++) {
        livingGenomes.insert(topSpecies[i].first);
        genomeToPopulation[topSpecies[i].first] = topSpecies[i].second;
    }

    string genealogyPath = getenv("HOME");
    string from = "/private";
    string to = "";
    
    size_t start_pos = genealogyPath.rfind(from.c_str());
    if (start_pos != std::string::npos) {
        genealogyPath.replace(start_pos, from.length(), to);
    }
    genealogyPath += "/Documents";

    string srcPath = FileSystem::getResourcePath();
    srcPath += "res/genealogyTemplate.html";

    ifstream inStream;
    inStream.open(srcPath.c_str());
    std::string contents( (std::istreambuf_iterator<char>(inStream) ),
                        (std::istreambuf_iterator<char>()) );

    inStream.close();
    
    genealogyPath += "/genealogy.html";
    
    std::stringstream treeStream;
    critter_width = segment_width * (getMaxGenomeLength(pTree)+ 1);
    if (critter_width < MIN_CRITTER_WIDTH)
        critter_width = MIN_CRITTER_WIDTH;

    set<string> addedGenomes;
    printTree(pTree, livingGenomes, genomeToPopulation, addedGenomes, treeStream);
    
    contents = replaceAll(contents, string("[[generation_height]]"), toString(critter_height * 1.4f));
    contents = replaceAll(contents, string("[[turn_height_multiplier]]"), toString(.1f));
    contents = replaceAll(contents, string("[[contents]]"), treeStream.str());
    contents = replaceAll(contents, string("[[critter_width]]"), toString(critter_width));
    contents = replaceAll(contents, string("[[critter_height]]"), toString(critter_height));
    contents = replaceAll(contents, string("[[segment_width]]"), toString(segment_width));
    contents = replaceAll(contents, string("[[segment_height]]"), toString(segment_height));

    string resPath = string("file://") + FileSystem::getResourcePath() + "res";
    contents = replaceAll(contents, string("[[res_path]]"), resPath);
    
    ofstream f;
    f.open(genealogyPath.c_str());
    f << contents;
    f.close();

    delete pTree;    
    
    openURL(genealogyPath.c_str(), false);
}
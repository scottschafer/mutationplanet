/************************************************************************
 MutationPlanet
 Copyright (C) 2012, Scott Schafer, scott.schafer@gmail.com
 
 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU Genekillatral Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.
 ************************************************************************/


/**
 Main
 
 Handles initialization, UI and rendering.
 
 Handling all the UI in one class is really not ideal, but it works reasonably well.
 If the UI were to be extended, a refactoring would be in order. I haven't explored
 extending the gameplay controls, but creating simple image buttons would be very nice
 to have.
 **/

#include "Main.h"
#include "SphereWorld.h"
#include "InstructionSet.h"
#include "Agent.h"
#include "UtilsRandom.h"
#include "Parameters.h"
#include "ScalableSlider.h"


#if TARGET_IPHONE_SIMULATOR||TARGET_OS_IPHONE
    #define USE_ZYWEBVIEW
#endif

#ifdef USE_ZYWEBVIEW
    #include "webview/ZYWebView.h"
    ZYWebView * pWebView = NULL;
#endif

//Analytics analytics;
// indexes of special graphics in our mSegmentBatch array

// Declare our game instance
Main game;
SphereWorld Main :: world;

static int mFollowingIndex;
float gTurnsPerSecond = 60;
int gLastFPS = 100;

// for tracking the turns per second
long numTurns = 0;
float totalElapsedTime = 0;

// the world runs in a different thread than the UI
pthread_mutex_t worldLockMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t topSpeciesLockMutex = PTHREAD_MUTEX_INITIALIZER;
bool threadAlive = false;

static vector<std::pair<std::string,int> > gTopSpecies;

map<string,string> mapChildToParentGenomes;


inline long curMS() {
    return clock() / (CLOCKS_PER_SEC/1000);
}

long killMS = 0;

LockWorldMutex::LockWorldMutex(bool bDoLock, pthread_mutex_t * mutex) : mDoLock(bDoLock)
{
    mMutex = (mutex == NULL) ? & worldLockMutex : mutex;

    if (mDoLock) {
	    pthread_mutex_lock( mMutex );
    }
}

LockWorldMutex::~LockWorldMutex()
{
	if (mDoLock) {
	    pthread_mutex_unlock( mMutex );
	}
}


/**
 * When useGenomeColorMapping is on, draw critters with the same genome i the same color. This makes it easier
 * to visualize the dominance of one particular genome.
 */
bool useGenomeColorMapping = true;

Vector4 getColorForGenome(const char *pGenome);
Vector4 getColorForGenome(const char *pGenome)
{
    string genome(pGenome);
    if (strlen(pGenome) == 1 && *pGenome == eInstructionPhotosynthesize)
        return Vector4(0,1,0,1);
    
    static std::map<std::string, Vector4> assignedColors;
    static std::vector<Vector4> colors;

    if (assignedColors.size() == 0)
        for (float r = 1; r >= .4f; r -= .3f)
            for (float g = 1; g >= .4f; g -= .3f)
                for (float b = 1; b >= .4f; b -= .3f)
                    if (r != 0 || g != 1 || b != 0)
                        colors.push_back(Vector4(r,g,b,1));
    
    static int colorIndex = 0;
    
    Vector4 result;
    if (assignedColors.find(genome) == assignedColors.end())
    {
        result = colors[colorIndex];
        assignedColors[genome] = result;
        colorIndex = (colorIndex + 1) % colors.size();
    }
    else
    {
        result = assignedColors[genome];
    }
    return result;
}

Main::Main()
{
    mShowingWebPage = mShowingInsertCritter = mShowingLoadSave = false;
    mViewScale = 1;
	mCurBarriers = 0;
    Parameters::instance.reset();
	mFollowingIndex = -1;

	_formSaveLoad = NULL;
        
    memset(mSegmentBatch, 0, sizeof(mSegmentBatch));
    for (int i = 0; i < sizeof(world.mAgents)/sizeof(world.mAgents[0]); i++)
    {
        Agent * pAgent = &world.mAgents[i];
        pAgent->mStatus = eNonExistent;
        for (int j = 0; j < sizeof(pAgent->mSegments)/sizeof(pAgent->mSegments[0]); j++)
        {
            SphereEntity * pEntity = &pAgent->mSegments[j];
            pEntity->mAgent = pAgent;
        }
    }
}

Vector3 getRandomSpherePoint();
Vector3 getRandomSpherePoint()
{
    Vector3 v(UtilsRandom::getUnitRandom(), UtilsRandom::getUnitRandom(), UtilsRandom::getUnitRandom());
    v.normalize();
    
    return v;
}

/**
 * The thread function that runs the logic of the world, including all critter processing
 **/
void * Main :: threadFunction(void*)
{
    threadAlive = true;
    while (threadAlive)
    {
		numTurns += (Parameters::instance.speed == 10) ? HYPER_NUM_STEPS : 1;

		long sleepMS = 0;
        if (Parameters::instance.speed == 0 || game.mShowingInsertCritter || game.mShowingWebPage || game.mShowingLoadSave)
        {
            // stopped
            sleepMS = 100;
			gTurnsPerSecond = 0;
        }
        else
        {
			// current time, elapsed time
			long curTicks = curMS();

			static long lastTicks = 0;
			if (lastTicks == 0)
				lastTicks = curTicks;
			long elapsedTicks = curTicks - lastTicks;
			lastTicks = curTicks;

			const long sampleTime = 250;

			static long elapsedTimeSinceTally = 0;
			static long startSampleTurns = 0;

			if (true)
			{
				LockWorldMutex m;

				if (elapsedTimeSinceTally > sampleTime)
				{
					int numTurnsSinceTally = (int)(numTurns - startSampleTurns);
					startSampleTurns = numTurns;            
					gTurnsPerSecond = ((float) numTurnsSinceTally) / ((float)elapsedTimeSinceTally) * 1000.f;
					world.sampleTopSpecies();

                    LockWorldMutex m2(true, &topSpeciesLockMutex);
                    const vector<std::pair<std::string,int> > & topSpecies = world.getTopSpecies();
                    gTopSpecies.clear();
                    for (vector<std::pair<std::string,int> >::const_iterator i = topSpecies.begin(); i != topSpecies.end(); i++) {
                        gTopSpecies.push_back(std::pair<std::string,int>(i->first, i->second));
                    }

                    elapsedTimeSinceTally = 0;
				}

				elapsedTimeSinceTally += elapsedTicks;

				int numSegments = world.step();
				static int lastFollowing = -1;
				mFollowingIndex = world.getTopCritterIndex();

#if 1
                if ((numSegments > KILL_SEGMENT_THRESHHOLD || gLastFPS < MIN_FPS) && (numSegments > MAX_TOTAL_SEGMENTS/5)) {
                    if (killMS == 0) {
                        killMS = curMS();
                    }
                    world.killAtLeastNumSegments(numSegments / 2, mFollowingIndex);
				}
#endif
			}

			long elapsedTicksInFuc = curMS() - curTicks;
            
            long expectedTicks = (10 - Parameters::instance.speed);
            expectedTicks = expectedTicks*expectedTicks*expectedTicks / 2;
            
			//print("expectedTicks = %d, actual ticks = %d\n", (int) expectedTicks, elapsedTicksInFuc);

            sleepMS = expectedTicks - elapsedTicksInFuc;
        }

		if (sleepMS > 0) {
			usleep(sleepMS * 1000);
		}

    }
    return NULL;
}

Rectangle Main::scaleUI(Rectangle r)
{
    return Rectangle(mUIScale*r.x, mUIScale*r.y, mUIScale * r.width, mUIScale * r.height);
}

typedef struct {
	int iSegmentType;
	const char * resSource;
} SegmentResourceMapping;

Theme * segmentTheme = NULL;

SegmentResourceMapping arraySegments[] = 
{
    iSpriteSphere, "res/sphere.png",
    iSpriteSphereRed, "res/sphere_red.png",
    iActiveSegment, "res/ActiveSegment.png",
	iActiveSegmentConditionOff, "res/ActiveSegmentConditionOff.png",
    iGenericSegment, "res/segment.png",
    iSegmentFrame, "res/segmentFrame.png",
    iMoveArrow, "res/MoveArrow.png",

    iSegmentIf, "res/segment_exec_if.png",
    iSegmentIfNot, "res/segment_exec_no.png",

    eBarrier1, "res/barrier.png",
	eBarrier2, "res/barrier.png",
	eBarrier3, "res/barrier.png",
	eBarrier4, "res/divider.png",
    
    eInstructionPhotosynthesize, "res/food.png",
    eInstructionMoveAndEat, "res/segment_M.png",
    eInstructionMove, "res/segment_n.png",
    eInstructionHyper, "res/segment_hyper.png",
    eInstructionSleep, "res/segment_Z.png",
    eInstructionTurnLeft, "res/segment_lt.png",
    eInstructionTurnRight, "res/segment_rt.png",
    eInstructionHardTurnLeft, "res/segment_[.png",
    eInstructionHardTurnRight, "res/segment_].png",
    
    eInstructionTestSeeFood, "res/segment_test_see_food.png",
    eInstructionTestBlocked, "res/segment_test_blocked.png",
    eInstructionTestOccluded, "res/segment_test_occluded.png",
    eInstructionTestPreyedOn, "res/segment_test_preyed_on.png",
	eInstructionTestTouchedSelf, "res/segment.png",

    eInstructionSetAnchored, "res/segment_anchor.png"
};

/**
 Initialize the UI, resources and kick off the world's logic thread
 **/
void Main::initialize()
{
	world.test();

#ifndef _WINDOWS
    if (_height > _width) {
        int swap = _width;
        _width = _height;
        _height = swap;
        _viewport.width = _width;
        _viewport.height = _height;
    }
#endif

	mUIScale = (float)max(getWidth(),getHeight()) / 1024;

    registerGesture(Gesture::GestureEvent(Gesture::GESTURE_PINCH));
    
    _font = Font::create((mUIScale == 1) ? "res/modata18.gpb" : "res/modata36.gpb");
    
    InstructionSet::reset();
    createUI();
    
	int i;
    for (i = 0; i < 255; i++)
        mSegmentBatch[i] = NULL;

	
	//segmentTheme = Theme::create("res/packedsegments.theme");
	for (i = 0; i < sizeof(arraySegments)/sizeof(arraySegments[0]); i++)
	{
        int initialCapacity = 0;
        if ((i >= iGenericSegment) && (i <= eInstructionClearAnchored))
            initialCapacity = 65530;

		mSegmentBatch[arraySegments[i].iSegmentType] = SpriteBatch::create(arraySegments[i].resSource, NULL, initialCapacity);
		Texture * pTexture = mSegmentBatch[arraySegments[i].iSegmentType]->getSampler()->getTexture();

		mSegmentSrcRect[arraySegments[i].iSegmentType] = Rectangle(0,0, pTexture->getWidth(), pTexture->getHeight());
	}
    

    resetWorld();
    
    int rc1;
    
    if( (rc1=pthread_create( &mThread, NULL, &Main::threadFunction, NULL)) )
    {
        printf("Thread creation failed: %d\n", rc1);
    }
}

void Main::drawSplash(void* param)
{
    clear(CLEAR_COLOR_DEPTH, Vector4(0, 0, 0, 1), 1.0f, 0);
    SpriteBatch* batch = SpriteBatch::create("res/logo_powered_mp.png");
    batch->start();
    batch->draw(getWidth() * 0.5f, getHeight() * 0.5f, 0.0f, 512.0f, 512.0f, 0.0f, 1.0f, 1.0f, 0.0f, Vector4::one(), true);
    batch->finish();
    SAFE_DELETE(batch);
}

/**
 Reset the world by killing off all critters and reseeding it with photosynthesize critters
 **/
void Main::resetWorld()
{
	LockWorldMutex m;
    
	world.clear();
    
    string genome;
    bool bAllowMutation = true;
    int initialCount = 500;
    

    if (false) {
        genome += (char)(eInstructionPhotosynthesize | eAlways);
        genome += (char)(eInstructionPhotosynthesize | eAlways);
        genome += (char)(eInstructionPhotosynthesize | eAlways);
        genome += (char)(eInstructionPhotosynthesize | eAlways);
        genome += (char)(eInstructionPhotosynthesize | eAlways);
        genome += (char)(eInstructionTurnRight | eAlways);
        genome += (char)(eInstructionMove | eAlways);


        /*
        genome += (char)(eInstructionTestSeeFood | eAlways);
		genome += (char)(eInstructionMoveAndEat | eIf);
		genome += (char)(eInstructionTurnLeft | eNotIf);
		genome += (char)(eInstructionMove | eAlways);
         */
        Agent *pAgent = world.createEmptyAgent();
        pAgent->initialize(Vector3(0,0,1), genome.c_str(), true);
        world.addAgentToWorld(pAgent);

        /*
		for (int i = 0; i < 1000; i++) {
			genome = eInstructionPhotosynthesize;
			Agent *pAgent = world.createEmptyAgent();
			pAgent->initialize(getRandomSpherePoint(), genome.c_str(), false);
			// randomize the initial lifespan so they don't all die at once
			pAgent->mLifespan = UtilsRandom::getRangeRandom(pAgent->mLifespan/2, pAgent->mLifespan*2);
			world.addAgentToWorld(pAgent);
		}
         */
	}
    else if (true) {
        genome += eInstructionPhotosynthesize;
        Agent *pAgent = world.createEmptyAgent();
        pAgent->initialize(Vector3(0,0,1), genome.c_str(), bAllowMutation);
        world.addAgentToWorld(pAgent);
//        pAgent->die(&world);
    }
    else {
        genome += eInstructionPhotosynthesize;
    for (int i = 0; i < 500; i++)
    {
        Agent *pAgent = world.createEmptyAgent();
        pAgent->initialize(getRandomSpherePoint(), genome.c_str(), bAllowMutation);
        // randomize the initial lifespan so they don't all die at once
        pAgent->mLifespan = UtilsRandom::getRangeRandom(pAgent->mLifespan/2, pAgent->mLifespan*2);
        world.addAgentToWorld(pAgent);
    }
    }
    /*
    vector<Instruction> instructions;

    Agent *pAgent = world.createEmptyAgent();
    bAllowMutation = false;
    instructions.push_back(Instruction(eInstructionMove, eAlways));
    instructions.push_back(Instruction(eInstructionMove, eIf));
    instructions.push_back(Instruction(eInstructionMove, eNotIf));
    instructions.push_back(Instruction(eInstructionSleep, eAlways));
    instructions.push_back(Instruction(eInstructionMove, eAlways));
    instructions.push_back(Instruction(eInstructionPhotosynthesize, eAlways));
    instructions.push_back(Instruction(eInstructionPhotosynthesize, eAlways));
    instructions.push_back(Instruction(eInstructionPhotosynthesize, eAlways));
    instructions.push_back(Instruction(eInstructionPhotosynthesize, eAlways));
    instructions.push_back(Instruction(eInstructionPhotosynthesize, eAlways));
    instructions.push_back(Instruction(0, eAlways));

    pAgent->initialize(getRandomSpherePoint(), instructions.data(), bAllowMutation);
    pAgent->mEnergy = 100;
    world.addAgentToWorld(pAgent);
    */
}

void Main :: resetParameters()
{
    Parameters::instance.reset();
    setControlValues();
    updateControlLabels();
}


/**
 Turn on or off the barriers
 **/
void Main :: onBarriers()
{
	if (mCurBarriers != (int)_barriersSlider->getValue()) {
		mCurBarriers = _barriersSlider->getValue();
		setBarriers(0, (mCurBarriers == 1));
		setBarriers(1, (mCurBarriers == 2));
		setBarriers(2, (mCurBarriers == 3));
		updateControlLabels();
	}
}

void Main :: setBarriers(int type, bool bOn)
{
	LockWorldMutex m;
    static char barrierTypes[4] = { eBarrier1, eBarrier2, eBarrier3, eBarrier4 };
    char barrierType = barrierTypes[type];
    
    if (! bOn)
    {
        // remove specified barrier type
        for (int i = 0; i < world.mMaxLiveAgentIndex; i++)
        {
            Agent & agent = world.mAgents[i];
            if ((agent.mStatus == eInanimate) && (agent.mSegments[0].mType == barrierType))
                world.killAgent(i);
        }
    }
    
    if (bOn)
    {
        std::string genome;
        genome += barrierType;
        if (type == 0)
        {
            float barrierDistance = .008f;
            for (float a = -MATH_PI * .95f; a < MATH_PI * .95f; a += barrierDistance)
            {
                Agent *pAgent = world.createEmptyAgent(true);
                
                Vector3 v(cos(a),sin(a*20)/10,sin(a));
                v.normalize();
                pAgent->initialize(v, genome.c_str(), true);
                world.addAgentToWorld(pAgent);
                pAgent->mStatus = eInanimate;
                pAgent->mEnergy = pAgent->getSpawnEnergy();
            }
        }
        
        if (type == 1)
        {
            float barrierDistance = .1f;
			for (float y = -1; y < 1; y += barrierDistance) {
				for (float x = -1; x < 1; x += barrierDistance) {
					for (float z = -1; z < 1; z += barrierDistance) {
					Vector3 v(x,y,z);
					v.normalize();
					if (y > 0)
						continue;
                    Agent *pAgent = world.createEmptyAgent(true);
                    pAgent->initialize(v, genome.c_str(), true);
                    world.addAgentToWorld(pAgent);
                    pAgent->mStatus = eInanimate;
                    pAgent->mEnergy = pAgent->getSpawnEnergy();
					}
				}
			}
		}

		if (type == 2)
		{
            float barrierDistance = .1f;
            for (float a = -MATH_PI * .98f; a < MATH_PI * .98f; a += barrierDistance)
            {
                for (int i = 0; i < 6; i++)
                {
                    Agent *pAgent = world.createEmptyAgent(true);
                    Vector3 v;
                    
                    float size = .5;
                    switch (i)
                    
                    {
                        case 0:
                            v = Vector3(1,cos(a) * size,sin(a) * size);
                            break;
                        case 1:
                            v = Vector3(-1,cos(a) * size,sin(a) * size);
                            break;
                        case 2:
                            v = Vector3(cos(a) * size, 1, sin(a) * size);
                            break;
                        case 3:
                            v = Vector3(cos(a) * size, -1, sin(a) * size);
                            break;
                        case 4:
                            v = Vector3(cos(a) * size, sin(a) * size, 1);
                            break;
                        case 5:
                            v = Vector3(cos(a) * size, sin(a) * size, -1);
                            break;
                    }
                    v.normalize();
                    pAgent->initialize(v, genome.c_str(), true);
                    world.addAgentToWorld(pAgent);
                    pAgent->mStatus = eInanimate;
                    pAgent->mEnergy = pAgent->getSpawnEnergy();
                }
            }
        }

		if (type == 3)
        {
            float barrierDistance = .1f;
            for (float a = -MATH_PI; a < MATH_PI; a += barrierDistance)
            {
                Agent *pAgent = world.createEmptyAgent(true);
                
                Vector3 v(cos(a),0,sin(a));
                v.normalize();
                pAgent->initialize(v, genome.c_str(), true);
                world.addAgentToWorld(pAgent);
                pAgent->mStatus = eInanimate;
                pAgent->mEnergy = pAgent->getSpawnEnergy();
            }
		}
    }
}

void Main::finalize()
{
    
    //SAFE_RELEASE(_formAdvanced);
    SAFE_RELEASE(_formMain);
}

void Main::update(float elapsedTime)
{
	updateSaveLoad(elapsedTime);

	_formMain->update(elapsedTime);
    if (mShowingWebPage)
        _formClose->update(elapsedTime);
    _formHelp->update(elapsedTime);
    
    if (mShowingInsertCritter) {
        _formInsertCritter->update(elapsedTime);
    }

	if (mShowingLoadSave) {
		_formSaveLoad->update(elapsedTime);
	}

	handleFollowCritter(elapsedTime);
}

Rectangle Main :: getRectangleForPoint(Vector3 pt, float renderSize, float offsetX, float offsetY, float scaleSize)
{
    mViewRotateMatrix.transformPoint(&pt);
    Matrix viewScaleMatrix;
    pt.x *= mViewScale;
    pt.y *= mViewScale;
    pt.z *= mViewScale;
                    
    if (pt.z < 0)
        return Rectangle(); // cheap backface clipping
                    
    // we use an orthogonal projection, but with some fakery to make it look more 3D
    float cellSize = (Parameters::instance.getMoveDistance() * pt.z * 400 + 3)  * mUIScale;
    float x = ((pt.x) * (pt.z/5 + 1)) * .98;
    float y = ((pt.y) * (pt.z/5 + 1)) * .98;
	float multSize = (1.0f + (mViewScale - 1.0f) / 6.0f) * .98f;
    return Rectangle(offsetX + renderSize * (x + 1) / 2 - cellSize / 2,
                                offsetY + renderSize * (y + 1) / 2 - cellSize / 2,
                                cellSize * multSize * scaleSize, cellSize * multSize * scaleSize);
}

static float renderSize, offsetX, offsetY;

const int reserveBatchCount = 200;
static bool mBatchStarted[256];

void Main::beginRender(float elapsedTime) {
    // Clear the color and depth buffers
    clear(CLEAR_COLOR_DEPTH, Vector4::zero(), 1.0f, 0);
    int i;

    for (i = 0; i < 255; i++) {
        mSegmentBatchCount[i] = reserveBatchCount;
		mBatchStarted[i] = false;
	}
        
    // determine the size and position of the the world
    renderSize = std::min(getWidth(), getHeight()) * .9;
    offsetX = (getWidth() - renderSize) / 2;
    offsetY = (getHeight() - renderSize) / 2;
        
    // draw the planet, slightly scaled down so the critters on the edges seem to have some height...
    mRenderSphereSize = renderSize * mViewScale * .995;
    mSphereOffsetX = (getWidth() - mRenderSphereSize) / 2;
    mSphereOffsetY = (getHeight() - mRenderSphereSize) / 2;
        
    _arcball.setBounds(mRenderSphereSize, mRenderSphereSize);
}

void Main::renderSphere(float elapsedTime) {

    mSegmentBatch[iSpriteSphere]->start();
	mSegmentBatch[iSpriteSphere]->draw(Rectangle(mSphereOffsetX, mSphereOffsetY, mRenderSphereSize, mRenderSphereSize), Rectangle(0,0,1024,1024));
    mSegmentBatch[iSpriteSphere]->finish();

    const float flashTime = 2000;
    const float flashTimePeak = 500;
        
    if (killMS) {
            
        float elapsedTime = curMS() - killMS;
        if ((elapsedTime < 0) || (elapsedTime > flashTime)) {
            killMS = 0;

        }
        else {
            float f = (elapsedTime < flashTimePeak) ? (elapsedTime / flashTimePeak) : ((flashTime - elapsedTime) / (flashTime - flashTimePeak));
                
            float dim = mRenderSphereSize * (3.5 + f) / 4;
            float redX = mSphereOffsetX + mRenderSphereSize / 2 - dim/2;
            float redY = mSphereOffsetY + mRenderSphereSize / 2 - dim/2;
                
		    mSegmentBatch[iSpriteSphereRed]->start();
            mSegmentBatch[iSpriteSphereRed]->draw(Rectangle(redX, redY, dim, dim), Rectangle(0,0,1024,1024), Vector4(1,1,1,f * .65f));
		    mSegmentBatch[iSpriteSphereRed]->finish();
        }
    }
    static vector<Vector3> coldPoints;
    if (coldPoints.size() == 0) {
        for (float a = -MATH_PI; a < MATH_PI; a += .04f)
        {
            Vector3 v(cos(a),3.0f,sin(a));
            v.normalize();
            coldPoints.push_back(v);

            Vector3 v2(cos(a),-3.0f,sin(a));
            v2.normalize();
            coldPoints.push_back(v2);
        }
    }

    for (vector<Vector3>::iterator i = coldPoints.begin(); i != coldPoints.end(); i++)
    {
        Rectangle r = getRectangleForPoint(*i, renderSize, offsetX, offsetY, 0.6f);
        if (r.width == 0)
            continue;
        int iBatch = eBarrier4;

        Rectangle src = mSegmentSrcRect[iBatch];
            
        Vector4 color(1,1,1, .3f);
        draw(iBatch, r, src, color);
    }
}


void Main::renderCritters(float elapsedTime) {
	// draw all the visible agents in two passes, first drawing the segments, then drawing the
    // ornaments
    for (int pass = 1; pass <= 2; pass++)
    {
        if (pass == 2 && mViewScale < 1.5)
            break;
            
        for (int i = 0; i <= world.mMaxLiveAgentIndex; i++)
        {
            Agent & agent = world.mAgents[i];
            if (agent.mStatus != eNonExistent)
            {                
                for (int j = agent.mNumSegments-1; j >= 0; j--)
                {
                    SphereEntity *pEntity = &agent.mSegments[j];
                        
                    bool isPhotosynthesize = pEntity->mType == eInstructionPhotosynthesize;
                        
                    Vector3 pt = pEntity->mLocation;

                    Rectangle dst = getRectangleForPoint(pt, renderSize, offsetX, offsetY);
                    if (dst.width == 0)
                        continue;

                    if (i == mFollowingIndex && j == 0) {

                        Rectangle spotlight = getRectangleForPoint(agent.mSegments[0].mLocation, renderSize, offsetX, offsetY);
                        for (int k = 1; k < agent.mNumSegments; k++) {
                            Rectangle segmentRect = getRectangleForPoint(agent.mSegments[k].mLocation, renderSize, offsetX, offsetY);
                            Rectangle combined;
                            spotlight.combine(spotlight,segmentRect,&combined);
                            spotlight = combined;
                        }
                        float spotlightDiam = max(spotlight.width, spotlight.height) * 1.5;
                        float spotlightX = spotlight.x + spotlight.width / 2;
                        float spotlightY = spotlight.y + spotlight.height / 2;
                        spotlight.x = spotlightX - spotlightDiam/2;
                        spotlight.y = spotlightY - spotlightDiam/2;
                        spotlight.width = spotlight.height = spotlightDiam;

                        Vector4 color(1,1,.5f, .2f);
                        int iSpotlight = iGenericSegment;
                        Rectangle src = mSegmentSrcRect[iSpotlight];

                        draw(iSpotlight, spotlight, src, color);
                    }

                    float cellSize = (Parameters::instance.getMoveDistance() * pt.z * 400 + 3)  * mUIScale;
                    float alpha = 1;
                        
                    if (agent.mStatus == eAlive)
                        alpha = .25f + (float)agent.mEnergy/(float)agent.getSpawnEnergy();
                    if (alpha > 1) alpha = 1;
                   
                    float x = ((dst.left() + dst.right()) / 2 - (mSphereOffsetX + mRenderSphereSize / 4)) / mRenderSphereSize;
                    float y = ((dst.top() + dst.bottom()) / 2 - (mSphereOffsetY + mRenderSphereSize / 4)) / mRenderSphereSize;
                    float delta = 1.0f - Vector2(x, y).length() * .6f;
                    delta *= delta;
                    alpha *= delta;

                    Vector4 color(1,1,1, alpha);
                        
                    int iBatch;
                    if (useGenomeColorMapping && agent.mStatus == eAlive && ! isPhotosynthesize)
                    {
                        iBatch = iGenericSegment;
                        color = getColorForGenome(agent.mGenome);
                        color.w = alpha;
                    }
                    else
                    {
                        iBatch = pEntity->mType;
                    }

                    Rectangle src = mSegmentSrcRect[iBatch];
                        
                    if (pass == 1)
                    {
                        draw(iBatch, dst, src, color);
                    }
                        
                    // if we're zoomed in, show an indicator around the active cell
                    if (pass == 2)
                    {
                        if (!useGenomeColorMapping && agent.mStatus == eAlive) {
                            char instruction = agent.mGenome.getInstruction(j);
                            if (InstructionSet::instructionSupportsConditions(instruction)) {
                                char execType = agent.mGenome.getExecType(j);
                                int iExecType = 0;
                                if (execType == eIf)
                                    iExecType = iSegmentIf;
                                else if (execType == eNotIf)
                                    iExecType = iSegmentIfNot;

                                Rectangle src = mSegmentSrcRect[iExecType];
                                if (iExecType != 0)
                                    draw(iExecType, dst, src);
                            }
                        }

						// draw the active cell indicator
                        if ((agent.mNumSegments > 1) && (j == (agent.mActiveSegment + agent.mNumSegments - 1) % agent.mNumSegments))
                        {
                            Rectangle activeSegmentRect = dst;
                            int iSegmentFrame = agent.getCondition() ? iActiveSegment : iActiveSegmentConditionOff;
                            Rectangle src = mSegmentSrcRect[iSegmentFrame];
                            activeSegmentRect.inflate(activeSegmentRect.width / 10, activeSegmentRect.height / 10);
                            draw(iSegmentFrame, activeSegmentRect, src, Vector4(1,1,1, 1));
                        }
                            
                        // draw the move arrow
                        if (agent.getIsMotile() && (j == 0))
                        {
                            Vector3 moveVector3 = agent.mMoveVector;
                            mViewRotateMatrix.transformPoint(&moveVector3);
                            Vector2 moveVector(moveVector3.x, moveVector3.y);
                            moveVector.normalize();
                            moveVector *= 2;
                            float rotation = atan2(moveVector.y,moveVector.x);
                                
                            Rectangle moveArrowRect = dst;
                            moveArrowRect.inflate(moveArrowRect.width / 2, moveArrowRect.height / 2);
                            Rectangle src = mSegmentSrcRect[iMoveArrow];
                                
                            Vector3 dstV(moveArrowRect.x, moveArrowRect.y,0);
                            Vector2 rotPoint(0.5f, 0.5f);
                            rotation += 45 * MATH_PI / 180;
                            draw(iMoveArrow, dstV, moveArrowRect.width, moveArrowRect.height, 0, 0, 1, 1, Vector4(1,1,1,1),
                                            rotPoint, rotation);
                                
                        }
                    }
                }
            }
        }
    }
}

void Main::renderSpeciesCounts(float elapsedTime) {
    for (int i = 0; i < 255; i++)
        mSegmentBatchCount[i] -= reserveBatchCount;
        _font->start();

	if (! mShowingWebPage)
    {
        _font->drawText("Top Species", getWidth()-190 * mUIScale, 10 * mUIScale, Vector4(1,1,1,1));
        
        LockWorldMutex m2(true, &topSpeciesLockMutex);
        vector<std::pair<std::string,int> > & topSpecies = gTopSpecies;//world.getTopSpecies();
        // draw the top species
        for (size_t i = 0; i < topSpecies.size(); i++)
        {
            if (i == 31)
                break;
            char buf[200];
            float x = getWidth()-60 * mUIScale;
            float y = mUIScale * (40 + 20 * i);
                
            sprintf(buf, "%d", topSpecies[i].second);
            std::string genome = topSpecies[i].first;
            const char *pGenome = genome.c_str();
            Vector4 drawColor(1,1,1,1);
            if (useGenomeColorMapping)
            {
                drawColor = getColorForGenome(topSpecies[i].first.c_str());
            }
            _font->drawText(buf, x, y, drawColor);
                
            x -= 40 * mUIScale;
            int len = strlen(pGenome);
            for (int j = (len - 1); j >= 0; j --)
            {
                    
                char ch = pGenome[j] & eInstructionMask;
                eSegmentExecutionType execType = (eSegmentExecutionType) (pGenome[j] & eExecTypeMask);
                    
                Rectangle dst(x, y, 16 * mUIScale, 16 * mUIScale);
                Rectangle src = mSegmentSrcRect[ch];
                draw((int) ch, dst, src);

                if (InstructionSet::instructionSupportsConditions(ch))
                {
                    int iExecType = 0;
                    SpriteBatch * pExecType = NULL;
                    switch (execType) {

                        default:break;
                                
                        case eIf:
                            iExecType = iSegmentIf;
                            break;
                        case eNotIf:
                            iExecType = iSegmentIfNot;
                            break;
                    }
                        
                    if (iExecType) {
                        Rectangle src = mSegmentSrcRect[iExecType];
                        draw(iExecType, dst, src);
                    }
                }

                x -= 18 * mUIScale;
            }
        }
            
        // show the turns per second
        totalElapsedTime += elapsedTime;
            
        if (gTurnsPerSecond > 0)
        {
            char buf[200];
            sprintf(buf, "Turns/sec: %d, FPS: %d", (int) gTurnsPerSecond, (int)this->getFrameRate());

            _font->drawText(buf, getWidth()-250 * mUIScale, getHeight() - 30 * mUIScale, Vector4(1,1,1,1));
        }
        gLastFPS = (int) getFrameRate();
    }
    _font->finish();

}

void Main::renderForms(float elapsedTime) {
    _formHelp->draw();
    _formMain->draw();

    if (mShowingWebPage)
        _formClose->draw();
            
    renderInsertCritter();

    if (mShowingLoadSave) {
        _formSaveLoad->draw();
    }
}

void Main::finishRender(float elapsedTime) {
    for (int i = 0; i < 255; i++)
        if (mSegmentBatch[i] && mBatchStarted[i])
            mSegmentBatch[i]->finish();
	/*
	if (points.size() == 0)
    for( size_t i = 0; i < 1000; ++i )
    {
        Point pt;
        pt.x = 0 + (rand() % 1000);
        pt.y = 0 + (rand() % 1000);
        pt.r = rand() % 255;
        pt.g = rand() % 255;
        pt.b = rand() % 255;
        pt.a = 255;
        points.push_back(pt);
    }    
    glColor3ub( 255, 255, 255 );
    glEnableClientState( GL_VERTEX_ARRAY );
    glEnableClientState( GL_COLOR_ARRAY );
    glVertexPointer( 2, GL_FLOAT, sizeof(Point), &points[0].x );
    glColorPointer( 4, GL_UNSIGNED_BYTE, sizeof(Point), &points[0].r );
    glPointSize( 30.0 );
    glDrawArrays( GL_POINTS, 0, points.size() );
    glDisableClientState( GL_VERTEX_ARRAY );
    glDisableClientState( GL_COLOR_ARRAY );
	glFlush();
	*/

}

/**
 Render the world and the UI.
 */
void Main::render(float elapsedTime)
{
	// locking the world to render it is safer, but has a speed penalty.
	// Only do it if we are throttling the speed or are zoomed in.
    
//	LockWorldMutex m(Parameters::instance.speed < 10 || mViewScale > 2);

	try {
        {
            LockWorldMutex m;
            beginRender(elapsedTime);
            renderSphere(elapsedTime);
            renderCritters(elapsedTime);
            renderSpeciesCounts(elapsedTime);
        }
		finishRender(elapsedTime);

		renderForms(elapsedTime);

   	}
	catch (...)
	{
		cout << "exception during render";
	}
}

void Main::safeExit() {
    threadAlive = false;
    void *status;
    pthread_join (mThread, &status);
    
    exit();
}

bool isCommandDown = false;

void Main::keyEvent(Keyboard::KeyEvent evt, int key)
{
    if (key == 0x10ed) {
        isCommandDown = (evt == Keyboard::KEY_PRESS);
    }
    
    if (evt == Keyboard::KEY_PRESS)
    {
        switch (key)
        {
            case Keyboard::KEY_W:
            case Keyboard::KEY_Q:
                if (isCommandDown)
                    safeExit();
                break;
                
            case Keyboard::KEY_ESCAPE:
                //safeExit();
                break;

			case Keyboard::KEY_UP_ARROW:
				mViewScale *= 1.1f;
				break;

			case Keyboard::KEY_DOWN_ARROW:
				mViewScale *= .9f;
				if (mViewScale < 1)
					mViewScale = 1;
				break;
        }
    }
}

static float gTouchStartViewScale;

void Main::touchEvent(Touch::TouchEvent evt, float x, float y, unsigned int contactIndex)
{
   static bool down = false;
    
    static Quaternion initialRotation;
    static Quaternion lastDragRotation;
    
    Vector2 sphereDisplayPoint(x - mSphereOffsetX, mRenderSphereSize - (y - mSphereOffsetY));
    switch (evt)
    {
        case Touch::TOUCH_PRESS:
            //if (! down)
            {
                gTouchStartViewScale = mViewScale;
                _arcball.click(sphereDisplayPoint);
                down = true;
            }
            break;
            
        case Touch::TOUCH_RELEASE:
            down = false;
            initialRotation = lastDragRotation;
            break;
            
        case Touch::TOUCH_MOVE: {
            gameplay::Quaternion q;
            _arcball.drag(sphereDisplayPoint, &q);
            
            q.multiply(initialRotation);
            lastDragRotation = q;
            Matrix::createRotation(q, &mViewRotateMatrix);
            break; }
    };
}

void Main::touchEvent(Touch::TouchEvent evt, int x, int y, unsigned int contactIndex)
{
	touchEvent(evt, (float) x, (float) y, contactIndex);
}

void Main::gesturePinchEvent(int x, int y, float scale)
{
    // the pinch on the mac Trackpad is entirely different than the pinch on the iOS screen...
    // so far, the only difference I'm seeing between platforms
#if TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR
    mViewScale = gTouchStartViewScale * scale;
#else
    mViewScale += scale;
#endif
    
    if (mViewScale > 5)
        mViewScale = 5;
    if (mViewScale < .5)
        mViewScale = .5;
}

const int FORM_HEIGHT_NO_ADVANCED = 326;
const int FORM_HEIGHT_WITH_ADVANCED = 750;

void Main::createUI()
{
    // Create main form
    _formMain = createForm(340, FORM_HEIGHT_NO_ADVANCED);
    _formMain->setPosition(4 * mUIScale, 8 * mUIScale);
    
    _speedSlider = createSliderControl(_formMain, "speed", "Speed:", 0, 10, 1);
    _mutationSlider = createSliderControl(_formMain, "mutation", "Mutation:", 0, 100);
	_barriersSlider = createSliderControl(_formMain, "barriers", "Barriers:", 0, 3);
    _randomFoodSlider = createSliderControl(_formMain, "randomFood", "Random Food:", 0, 100);
	_cellSizeSlider = createSliderControl(_formMain, "cellSize", "Cell size:", 1, 10, 1);

    _insertButton = createButton(_formMain, "Insert...");
    _genealogyButton = createButton(_formMain, "Genealogy...");
    _saveLoadButton = createButton(_formMain, "Save / Load / Reset...");
	_followCritter = createCheckboxControl(_formMain, "Follow top critter");
    
    _showAdvanced = createCheckboxControl(_formMain, "Show advanced settings");

	createSpacer(_formMain, 10);
	
    // the advanced form is now part of main form
    _formAdvanced = _formMain;

    _extraSpawnEnergyPerSegmentSlider = createSliderControl(_formAdvanced,"extraSpawnEnergyPerSegment", "Spawn energy:", 50, 2000);
	_deadCellDormancySlider = createSliderControl(_formAdvanced, "deadCellDormancy", "Sprout turns:", 100, 50000);
	_photoSynthesizeEnergyGainSlider = createSliderControl(_formAdvanced, "photoSynthesizeEnergyGain", "Photosynthesis:", 1.0f, 5.0f);
    _moveEnergyCostSlider = createSliderControl(_formAdvanced, "moveEnergyCost", "Move:", 0, 50);
    _moveAndEatEnergyCostSlider = createSliderControl(_formAdvanced, "moveAndEatEnergyCost", "Move & eat:", 0, 100);
    _mouthSizeSlider = createSliderControl(_formAdvanced, "mouthSize", "Mouth size:", .75f, 4.0f);

    _lookDistanceSlider = createSliderControl(_formAdvanced,"lookDistance", "Vision range:", 1, 100);

    _extraCyclesForMoveSlider = createSliderControl(_formAdvanced,"extraCyclesForMove", "Move delay:", 0, 20);
    _biteStrengthSlider = createSliderControl(_formAdvanced,"biteStrength", "Bite strength:", 0.1, 5);
	_digestionEfficiencySlider = createSliderControl(_formAdvanced, "digestionEfficiency", "Digestion:", 0.1, 1);

    _allowSelfOverlap = createCheckboxControl(_formAdvanced, "Allow self overlap");

	_starveBecomeFood = createCheckboxControl(_formAdvanced, "Starve => food");
	_cannibals = createCheckboxControl(_formAdvanced, "Allow cannibalism");

    _formMain->setConsumeInputEvents(false);
    _formAdvanced->setConsumeInputEvents(false);
    
    _formHelp = Form::create("res/editor.form");
    _formHelp->setPosition(720 * mUIScale, 660 * mUIScale);
    
    _formHelp->setSize(getWidth() - (unsigned int)_formHelp->getX(), getHeight() - (unsigned int)_formHelp->getY());
    Control * pHelpMain = _formHelp->getControl("main");
    pHelpMain->setSize(_formHelp->getWidth(), _formHelp->getHeight());
    Theme::Style * pNoBorder = _formHelp->getTheme()->getStyle("noBorder");
    _formHelp->setStyle(pNoBorder);
    _formHelp->getControl("main")->setStyle(pNoBorder);
    
    _helpButton = createButton(_formHelp, "What is this?", "help", Vector2(-1,-1), Vector2(290,40));
    _colorCodeSpecies = createCheckboxControl(_formHelp, "Color-code species");
	_colorCodeSpecies->setChecked(useGenomeColorMapping);
    _colorCodeSpecies->setFontSize(30 * mUIScale);
    _colorCodeSpecies->setTextAlignment(Font::ALIGN_VCENTER);
    
    // create insert critter form
	createInsertCritterForm();

	createLoadSaveForm();

	setControlValues();
    updateControlLabels();
    
    _formClose = createForm(1024, 768, true, false);
    _closeButton = createButton(_formClose, "Close");
    _formClose->setConsumeInputEvents(false);
    _formClose->setVisible(false);
	
}

void Main::controlEvent(Control* control, EventType evt)
{
#ifdef USE_ZYWEBVIEW
    if (pWebView != NULL) {
        pWebView->removeWebView();
        pWebView = NULL;
        mShowingWebPage = false;
        _formClose->setVisible(false);
        return;
    }
#endif
    
    
    switch(evt)
    {
        default:
            break;
            
        case Listener::CLICK:
            if (control == _resetWorldButton)
                resetWorld();
            else if (control == _resetParametersButton)
                resetParameters();
			else if (control == _saveLoadButton)
				handleSaveLoad();
            else if (control == _helpButton)
            {
                string path = FileSystem::getResourcePath();
                path += "res/index.html";

                this->openURL(path.c_str());
            }
            else if (control == _genealogyButton)
            {
                handleGenealogy();
            }
			else {
				if (! handleSaveLoadEvent(control, evt))
					handleInsertCritterEvent(control, evt);
			}
            break;
            
        case Listener::VALUE_CHANGED:
			if (control == _showAdvanced) {
				if (_showAdvanced->isChecked()) {
					_formMain->setSize(_formMain->getWidth(), FORM_HEIGHT_WITH_ADVANCED * mUIScale);
				}
				else {
					_formMain->setSize(_formMain->getWidth(), FORM_HEIGHT_NO_ADVANCED * mUIScale);
				}
				
				Container *pMainContainer = (Container*)_formMain->getControl("main");
				pMainContainer->setHeight(_formMain->getHeight());
			}
            else if (control == _colorCodeSpecies)
                useGenomeColorMapping = _colorCodeSpecies->isChecked();
            else if (control == _speedSlider)
                Parameters::instance.speed = _speedSlider->getValue();
            else if (control == _mutationSlider)
                Parameters::instance.mutationPercent = _mutationSlider->getValue();
            else if (control == _randomFoodSlider)
                Parameters::instance.randomFood = _randomFoodSlider->getValue();
            else if (control == _cellSizeSlider)
                Parameters::instance.cellSize = _cellSizeSlider->getValue();
            else if (control == _photoSynthesizeEnergyGainSlider)
                Parameters::instance.photoSynthesizeEnergyGain = _photoSynthesizeEnergyGainSlider->getValue();
			else if (control == _deadCellDormancySlider)
				Parameters::instance.deadCellDormancy = _deadCellDormancySlider->getValue();
            else if (control == _moveEnergyCostSlider)
                Parameters::instance.moveEnergyCost = _moveEnergyCostSlider->getValue();
            else if (control == _moveAndEatEnergyCostSlider)
                Parameters::instance.moveAndEatEnergyCost = _moveAndEatEnergyCostSlider->getValue();
			else if (control == _mouthSizeSlider)
				Parameters::instance.mouthSize = _mouthSizeSlider->getValue();
            else if (control == _baseSpawnEnergySlider)
                Parameters::instance.baseSpawnEnergy = _baseSpawnEnergySlider->getValue();
            else if (control == _extraSpawnEnergyPerSegmentSlider)
                Parameters::instance.extraSpawnEnergyPerSegment = _extraSpawnEnergyPerSegmentSlider->getValue();
            else if (control == _lookDistanceSlider)
                Parameters::instance.lookDistance = _lookDistanceSlider->getValue();
            else if (control == _allowSelfOverlap)
                Parameters::instance.allowSelfOverlap = _allowSelfOverlap->isChecked();
			else if (control == _extraCyclesForMoveSlider)
				Parameters::instance.extraCyclesForMove = _extraCyclesForMoveSlider->getValue();
			else if (control == _biteStrengthSlider)
				Parameters::instance.biteStrength = _biteStrengthSlider->getValue();
			else if (control == _digestionEfficiencySlider)
				Parameters::instance.digestionEfficiency = _digestionEfficiencySlider->getValue();
			else if (control == _starveBecomeFood)
				Parameters::instance.turnToFoodAfterDeath = _starveBecomeFood->isChecked();
			else if (control == _cannibals)
				Parameters::instance.cannibals = _cannibals->isChecked();
			else if (control == _barriersSlider)
				onBarriers();
			else if (control == _followCritter)
				world.setAllowFollow(_followCritter->isChecked());
            updateControlLabels();
            break;
    }
}


void Main :: updateControlLabel(std::string parameterId, const char *pFormat, ...)
{
    Label * pLabel = (Label*)_formMain->getControl((parameterId + "Label").c_str());
    if (! pLabel && _formAdvanced)
        pLabel = (Label*)_formAdvanced->getControl((parameterId + "Label").c_str());

	if (! pLabel && _formSaveLoad)
		pLabel = (Label*)_formSaveLoad->getControl((parameterId + "Label").c_str());
		
    if (pLabel)
    {
        char buffer[1024];
        va_list argptr;
        va_start(argptr, pFormat);
        vsprintf(buffer, pFormat, argptr);
        va_end(argptr);
        
        int len = strlen(buffer);
        if (len > 2 && buffer[len-1] == '0' && buffer[len-2] == '.')
            buffer[len-2] = 0;
        pLabel->setText(buffer);
    }}


void Main::updateControlLabels()
{
    
    updateControlLabel("speed", "%d", Parameters::instance.speed);
    updateControlLabel("mutation", "%d%%", Parameters::instance.mutationPercent);
    updateControlLabel("randomFood", "%d%%", Parameters::instance.randomFood);
	updateControlLabel("barriers", "%d", mCurBarriers);
    updateControlLabel("cellSize", "%d", (int) Parameters::instance.cellSize);
    updateControlLabel("photoSynthesizeEnergyGain", "+%.1f", Parameters::instance.photoSynthesizeEnergyGain);
	updateControlLabel("deadCellDormancy", "%d", Parameters::instance.deadCellDormancy/100);
    updateControlLabel("moveEnergyCost", "-%.1f", Parameters::instance.moveEnergyCost);
    updateControlLabel("moveAndEatEnergyCost", "-%.1f", Parameters::instance.moveAndEatEnergyCost);
    updateControlLabel("mouthSize", "%.1f", Parameters::instance.mouthSize);
	updateControlLabel("baseSpawnEnergy", "%d", (int) Parameters::instance.baseSpawnEnergy);
    updateControlLabel("extraSpawnEnergyPerSegment", "%d", (int) Parameters::instance.extraSpawnEnergyPerSegment);
    updateControlLabel("lookDistance", "%d", Parameters::instance.lookDistance);
    updateControlLabel("extraCyclesForMove", "%d", Parameters::instance.extraCyclesForMove);
	updateControlLabel("biteStrength", "%d%%", (int)(Parameters::instance.biteStrength * 100));
	updateControlLabel("digestionEfficiency", "%d%%", (int)(Parameters::instance.digestionEfficiency * 100));
	
}

void Main::setControlValues()
{
    _speedSlider->setValue(Parameters::instance.speed);
    _mutationSlider->setValue(Parameters::instance.mutationPercent);
    _randomFoodSlider->setValue(Parameters::instance.randomFood);
    _cellSizeSlider->setValue(Parameters::instance.cellSize);
    _photoSynthesizeEnergyGainSlider->setValue(Parameters::instance.photoSynthesizeEnergyGain);
	_deadCellDormancySlider->setValue(Parameters::instance.deadCellDormancy);
    _moveEnergyCostSlider->setValue(Parameters::instance.moveEnergyCost);
    _moveAndEatEnergyCostSlider->setValue(Parameters::instance.moveAndEatEnergyCost);
    _mouthSizeSlider->setValue(Parameters::instance.mouthSize);
    _extraSpawnEnergyPerSegmentSlider->setValue(Parameters::instance.extraSpawnEnergyPerSegment);
    _lookDistanceSlider->setValue(Parameters::instance.lookDistance);
    _allowSelfOverlap->setChecked(Parameters::instance.allowSelfOverlap);
	_starveBecomeFood->setChecked(Parameters::instance.turnToFoodAfterDeath);
	_cannibals->setChecked(Parameters::instance.cannibals);
	_extraCyclesForMoveSlider->setValue(Parameters::instance.extraCyclesForMove);
	_biteStrengthSlider->setValue(Parameters::instance.biteStrength);
	_digestionEfficiencySlider->setValue(Parameters::instance.digestionEfficiency);

	_colorCodeSpecies->setChecked(useGenomeColorMapping);
	this->_formAdvanced->update(1);
}

/**
 * Methods for programmatically creating the UI
 */
#define LABEL_WIDTH 130
#define SLIDER_WIDTH 140

Form * Main :: createForm(float width, float height, bool isLayoutVertical, bool framing)
{
    Form * result = Form::create(isLayoutVertical ? "res/layoutVertical.form" : "res/layoutAbsolute.form");
    Container *pMainContainer = (Container*)result->getControl("main");
    pMainContainer->setSkinColor(Vector4(0,0,0,1));
    
    result->setSize(width * mUIScale, height * mUIScale);
    pMainContainer->setSize(width * mUIScale, (height - 20) * mUIScale);
    
    float s = framing ? mUIScale : 0;
    
    pMainContainer->setPosition(pMainContainer->getX() * mUIScale, pMainContainer->getY() * mUIScale);
    {
        s = 0;
        const Theme::Margin& m = pMainContainer->getMargin();
        pMainContainer->setMargin(m.top * s, m.bottom * s, m.left * s, m.right * s);
        const Theme::Border& b = pMainContainer->getBorder();
        pMainContainer->setBorder(b.top * s, b.bottom * s, b.left * s, b.right * s);
    }
    {
        s = framing ? mUIScale : 0;
        const Theme::Margin& m = result->getMargin();
        result->setMargin(m.top * s, m.bottom * s, m.left * s, m.right * s);
        const Theme::Border& b = result->getBorder();
        result->setBorder(b.top * s, b.bottom * s, b.left * s, b.right * s);
    }
    return result;
}

Label * Main :: createControlHeader(Form *form, std::string text, Vector2 pos, Vector2 size, Vector4 textColor)
{
    Theme::Style * pNoBorder = form->getTheme()->getStyle("noBorder");
    
    Container *pMainContainer = (Container*)form->getControl("main");
    
    if (size.x == -1)
    {
        size.x = 300;
        size.y = (pMainContainer->getControls().size() ? 30 : 20);
    }
    
    Label *pLabel = Label::create("", pNoBorder);
    pLabel->setFontSize(pLabel->getFontSize()*mUIScale);
    pLabel->setZIndex(pMainContainer->getControls().size());
    pMainContainer->addControl(pLabel);
    pLabel->setTextAlignment(Font::ALIGN_BOTTOM_LEFT);
    pLabel->setText(text.c_str());
    pLabel->setSize(size.x * mUIScale, size.y * mUIScale);
    pLabel->setTextColor(textColor);
    
    if (pos.x != -1)
        pLabel->setPosition(pos.x * mUIScale, pos.y * mUIScale);
	return pLabel;
}

Container * Main :: createContainer(Form *form, bool border, Vector2 pos, Vector2 size)
{
    Theme::Style * pNoBorder = form->getTheme()->getStyle("noBorder");
    Theme::Style * pBasic = form->getTheme()->getStyle("basic");
    
    Container *pContainer = Container::create("", border ? pBasic : pNoBorder);
    Container *pMainContainer = (Container*)form->getControl("main");
    float height = (pMainContainer->getControls().size() ? 40 : 25) * mUIScale;
    pContainer->setSize(300 * mUIScale, height * mUIScale);
    pContainer->setZIndex(pMainContainer->getControls().size());
    pMainContainer->addControl(pContainer);
    
    if (pos.x != -1)
        pContainer->setPosition(pos.x * mUIScale, pos.y * mUIScale);
    
    if (size.x != -1)
        pContainer->setSize(size.x * mUIScale, size.y * mUIScale);
    
    return pContainer;
}

void Main :: createSpacer(Form *form, float height)
{
    Theme::Style * pNoBorder = form->getTheme()->getStyle("noBorder");
    
    Container *pContainer = Container::create("", pNoBorder);
    Container *pMainContainer = (Container*)form->getControl("main");
    pContainer->setSize(300 * mUIScale, height * mUIScale);
    pContainer->setZIndex(pMainContainer->getControls().size());
    pMainContainer->addControl(pContainer);
}

Slider * Main :: createSliderControl(Form *form, std::string id, std::string label, float minValue, float maxValue, float step /*= 0 */)
{
    Theme::Style * pNoBorder = form->getTheme()->getStyle("noBorder");
    
    Container *pContainer = Container::create("", pNoBorder);
    Container *pMainContainer = (Container*)form->getControl("main");
    pContainer->setZIndex(pMainContainer->getControls().size());
    pMainContainer->addControl(pContainer);
    pContainer->setSize(350 * mUIScale, 31 * mUIScale);
    
    Label *pLabel = Label::create("", pNoBorder);
    pContainer->addControl(pLabel);
    pLabel->setTextAlignment(Font::ALIGN_TOP_LEFT);
    pLabel->setText(label.c_str());
    pLabel->setFontSize(pLabel->getFontSize() * mUIScale);
    pLabel->setSize(LABEL_WIDTH * mUIScale, 35 * mUIScale);
    pLabel->setPosition(5 * mUIScale, 0);
    
    Slider *pSlider = ScalableSlider::create(id.c_str(), pNoBorder, mUIScale);
    pContainer->addControl(pSlider);
    pSlider->setSize(SLIDER_WIDTH * mUIScale, 35 * mUIScale);
    pSlider->setPosition(LABEL_WIDTH * mUIScale,-8 * mUIScale);
    pSlider->setMin(minValue);
    pSlider->setMax(maxValue);
    pSlider->setStep(step);
    
    Label *pValueLabel = Label::create((id + "Label").c_str(), pNoBorder);
    pContainer->addControl(pValueLabel);
    pValueLabel->setTextAlignment(Font::ALIGN_TOP_LEFT);
    pValueLabel->setSize(60 * mUIScale, 30 * mUIScale);
    pValueLabel->setFontSize(pValueLabel->getFontSize() * mUIScale);
    pValueLabel->setPosition((SLIDER_WIDTH + LABEL_WIDTH) * mUIScale, 0);
    
    pSlider->addListener(this, Listener::VALUE_CHANGED);
    
    return pSlider;
}


Label * Main :: createLabel(Form *form, std::string text, const char * pId, Vector2 pos, Vector2 size)
{
	Vector4 textColor(1,1,1,1);
	Theme::Style * pNoBorder = form->getTheme()->getStyle("noBorder");
    
    Container *pMainContainer = (Container*)form->getControl("main");
    
    if (size.x == -1)
    {
        size.x = 300;
        size.y = (pMainContainer->getControls().size() ? 30 : 20);
    }
    
	Label *pLabel = Label::create((string(pId)+"Label").c_str(), pNoBorder);
    pLabel->setFontSize(pLabel->getFontSize()*mUIScale);
    pLabel->setZIndex(pMainContainer->getControls().size());
    pMainContainer->addControl(pLabel);
    pLabel->setTextAlignment(Font::ALIGN_BOTTOM_LEFT);
    pLabel->setText(text.c_str());
    pLabel->setSize(size.x * mUIScale, size.y * mUIScale);
    pLabel->setTextColor(textColor);
    
    if (pos.x != -1)
        pLabel->setPosition(pos.x * mUIScale, pos.y * mUIScale);
	
	return pLabel;
}

CheckBox * Main :: createCheckboxControl(Form *form, std::string label, Vector2 pos, Vector2 size)
{
    Theme::Style * pNoBorder = form->getTheme()->getStyle("noBorder");
    
    Container *pContainer = Container::create("", pNoBorder);
    Container *pMainContainer = (Container*)form->getControl("main");
    
    if (size.x == -1)
    {
        size.x = 300;
        size.y = 30;
    }
    
    CheckBox *pCheckbox = CheckBox::create("", pNoBorder);
    pCheckbox->setZIndex(pMainContainer->getControls().size());
    pCheckbox->setTextAlignment(Font::ALIGN_BOTTOM_LEFT);
    pMainContainer->addControl(pCheckbox);
    pCheckbox->setText(label.c_str());
    pCheckbox->setSize(size.x * mUIScale, size.y * mUIScale);
//    pCheckbox->setSize(<#float width#>, <#float height#>)
//	pCheckbox->setImageSize(size.y * mUIScale, size.y * mUIScale);
    pCheckbox->setFontSize(pCheckbox->getFontSize() * mUIScale);
    
    pCheckbox->addListener(this, Listener::VALUE_CHANGED);
    
    if (pos.x != -1)
        pCheckbox->setPosition(pos.x * mUIScale, pos.y * mUIScale);
    
    return pCheckbox;
}

Button * Main :: createButton(Form *form, std::string label, const char * id, Vector2 pos, Vector2 size)
{
    Theme::Style * pNoBorder = form->getTheme()->getStyle("noBorder");
    Theme::Style * pBasic = form->getTheme()->getStyle("basic");
    
    if (size.x == -1)
    {
        size.x = 318;
        size.y = 40;
    }
    
    Container *pMainContainer = (Container*)form->getControl("main");
    
    Button *pButton = Button::create(id, pBasic);
    pButton->setZIndex(pMainContainer->getControls().size());
    pMainContainer->addControl(pButton);
    pButton->setText(label.c_str());
    pButton->setSize(size.x * mUIScale, size.y * mUIScale);
	pButton->setMargin(0,0,0,0);
    
    pButton->addListener(this, Listener::CLICK);
    
    if (pos.x != -1)
        pButton->setPosition(pos.x * mUIScale, pos.y * mUIScale);
    
    pButton->setFontSize(24*mUIScale);
    return pButton;
}


SpriteBatch * Main :: getLegalSpriteBatch(int iBatch)
{
	SpriteBatch *pResult = mSegmentBatch[iBatch];
	if (! pResult)
	{
		iBatch = iGenericSegment;
		pResult = mSegmentBatch[iBatch];
	}

	if (mSegmentBatchCount[iBatch] >= 8000)
	{
		pResult->finish();
		pResult->start();
		mSegmentBatchCount[iBatch] = reserveBatchCount;
		return pResult;
	}

	if (! mBatchStarted[iBatch])
	{
		mBatchStarted[iBatch] = true;
		pResult->start();
	}
	++mSegmentBatchCount[iBatch];
	return pResult;
}


void Main :: draw(int iBatch, const Rectangle& dst, const Rectangle& src, const Vector4& color)
{
//	if (iBatch == iGenericSegment || iBatch == eInstructionPhotosynthesize)
//		return;

	SpriteBatch * pBatch = getLegalSpriteBatch(iBatch);
	if (pBatch)
		pBatch->draw(dst, src, color);
}

void Main :: draw(int iBatch, const Vector3& dst, const Rectangle& src, const Vector2& scale, const Vector4& color)
{
	SpriteBatch * pBatch = getLegalSpriteBatch(iBatch);
	if (pBatch)
		pBatch->draw(dst, src, scale, color);
}

void Main :: draw(int iBatch, const Vector3& dst, const Rectangle& src, const Vector2& scale, const Vector4& color,
            const Vector2& rotationPoint, float rotationAngle)
{
	SpriteBatch * pBatch = getLegalSpriteBatch(iBatch);
	if (pBatch)
		pBatch->draw(dst, src, scale, color, rotationPoint, rotationAngle);
}

void Main :: draw(int iBatch, const Vector3& dst, float width, float height, float u1, float v1, float u2, float v2, const Vector4& color,
            const Vector2& rotationPoint, float rotationAngle, bool positionIsCenter)
{
	SpriteBatch * pBatch = getLegalSpriteBatch(iBatch);
	if (pBatch)
		pBatch->draw(dst, width, height, u1, v1, u2, v2, color, rotationPoint, rotationAngle, positionIsCenter);
}

void Main :: draw(int iBatch, float x, float y, float z, float width, float height, float u1, float v1, float u2, float v2, const Vector4& color,
            const Vector2& rotationPoint, float rotationAngle, bool positionIsCenter)
{
	SpriteBatch * pBatch = getLegalSpriteBatch(iBatch);
	if (pBatch)
		pBatch->draw(x, y, z, width, height, u1, v1, u2, v2, color, rotationPoint, rotationAngle, positionIsCenter);
}

void Main :: draw(int iBatch, const Vector3& position, const Vector3& right, const Vector3& forward, float width, float height, 
            float u1, float v1, float u2, float v2, const Vector4& color, const Vector2& rotationPoint, float rotationAngle)
{
	SpriteBatch * pBatch = getLegalSpriteBatch(iBatch);
	if (pBatch)
		pBatch->draw(position, right, forward, width, height, u1, v1, u2, v2, color, rotationPoint, rotationAngle);
}

void Main :: draw(int iBatch, float x, float y, float width, float height, float u1, float v1, float u2, float v2, const Vector4& color)
{
	SpriteBatch * pBatch = getLegalSpriteBatch(iBatch);
	if (pBatch)
		pBatch->draw(x, y, width, height, u1, v1, u2, v2, color);
}

void Main :: draw(int iBatch, float x, float y, float width, float height, float u1, float v1, float u2, float v2, const Vector4& color, const Rectangle& clip)
{
	SpriteBatch * pBatch = getLegalSpriteBatch(iBatch);
	if (pBatch)
		pBatch->draw(x, y, width, height, u1, v1, u2, v2, color, clip);
}

void Main :: draw(int iBatch, float x, float y, float z, float width, float height, float u1, float v1, float u2, float v2, const Vector4& color, bool positionIsCenter)
{
	SpriteBatch * pBatch = getLegalSpriteBatch(iBatch);
	if (pBatch)
		pBatch->draw(x, y, z, width, height, u1, v1, u2, v2, color, positionIsCenter);
}

void Main :: handleFollowCritter(float elapsedTime)
{
	if (! world.isFollowing()) {
		_followCritter->setChecked(false);
		return;
	}

	if (! _followCritter->isChecked()) {
		return;
	}

//	LockWorldMutex m;

	mFollowingIndex = world.getTopCritterIndex();
	if (mFollowingIndex == -1)
		return;

	Agent & agent = world.mAgents[mFollowingIndex];

	float renderSize = std::min(getWidth(), getHeight()) * .9;
    float offsetX = (getWidth() - renderSize) / 2;
    float offsetY = (getHeight() - renderSize) / 2;

	if (mViewScale < 2.0f) {
		float scale = (1.0f + elapsedTime / 1000.0f);
		mViewScale *= scale;
	}

	for (int loop = 0; loop < 4; loop++) {
		Vector3 loc = agent.mSegments[0].mLocation;
		Rectangle r = getRectangleForPoint(loc, renderSize, offsetX, offsetY, 1);

		float scrollDelay = 2000.0f;

        float margin = .25f;
        float minX = getWidth() * margin;
        float minY = getHeight() * margin;
        float maxX = getWidth() * (1.0f - margin);
        float maxY = getHeight() * (1.0f - margin);

        if (r.width != 0 && (r.left() > minX) && (r.right() < maxX) && (r.top() > minY) && (r.bottom() < maxY)) {
			float rX = (r.left() + r.right()) / 2;
			float rY = (r.top() + r.bottom()) / 2;
			float dX = (rX - getWidth()) / 2;
			float dY = (rY - getHeight()) / 2;

			float delta = sqrtf(dX*dX + dY*dY);
			if (delta < 10)
				return;

			dX = getWidth()/2;
			dY = getHeight()/2;
			float maxDelta = sqrtf(dX*dX + dY*dY);

			scrollDelay *= maxDelta / delta;
		}

		if (r.width == 0) {
			loc.x = loc.y = 0;
		}
		else {
			loc.x = r.left() + r.width / 2;
			loc.y = r.top() + r.height / 2;
		}

		float xOff = (loc.x - getWidth()/2);
		float yOff = (loc.y - getHeight()/2);
	
	
		xOff *= elapsedTime/scrollDelay;
		yOff *= elapsedTime/scrollDelay;

		float x = getWidth() / 2;
		float y = getHeight() / 2;

		if (fabsf(xOff) > 1 || fabsf(yOff) > 1) {

			touchEvent(Touch::TOUCH_PRESS, x, y, 0);

			x -= xOff;
			y -= yOff;
			touchEvent(Touch::TOUCH_MOVE, x, y, 0);
			touchEvent(Touch::TOUCH_RELEASE, x, y, 0);
		}
	}
}

#ifdef _WINDOWS
#define WIN32_LEAN_AND_MEAN 
#include <Windows.h>
#endif

void Main :: openURL(const char *pPath, bool externalBrowser /* = false */)
{
#ifdef USE_ZYWEBVIEW
    if (! externalBrowser) {
        if (pWebView == NULL) {
            pWebView = new ZYWebView();
            pWebView->init();
            
            float x = 0;
            float y = 40;
            float w = getWidth();
            float h = getHeight() - y;
            pWebView->showWebView(pPath, x, y, w, h);
            mShowingWebPage = true;
            _formClose->setVisible(true);
        }
        return;
    }
#endif
    
#ifdef _WINDOWS
	extern void winLaunchFile(const char *pFile);

	winLaunchFile(pPath);

//    string command = "open ";
//    command += pPath;
//    system(command.c_str());
#else

#ifdef __APPLE__
//    this->launchURL(url.c_str());
#else
//    this->launchURL(url.c_str());
#endif
#endif
}

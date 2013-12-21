/************************************************************************
 MutationPlanet
 Copyright (C) 2012, Scott Schafer, scott.schafer@gmail.com
 
 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
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

// indexes of special graphics in our mSegmentBatch array
enum
{
    iHelpPage1 = 0,
    iHelpPage2,
    iHelpPage3,
    iHelpPage4,
    iHelpPage5,
    iHelpClose,
    
    iSpriteSphere,
    
    iActiveSegment,
    iGenericSegment,


    iSegmentFrame,
    iMoveArrow,

	iSegmentIf = 200,
	iSegmentIfNot,
	iSegmentAlways
};

// Declare our game instance
Main game;
static SphereWorld world;

// for tracking the turns per second
int numTurns = 0;
float totalElapsedTime = 0;

// the world runs in a different thread than the UI
pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;
bool threadAlive = false;

/**
 * When useGenomeColorMapping is on, draw critters with the same genome i the same color. This makes it easier
 * to visualize the dominance of one particular genome.
 */
bool useGenomeColorMapping = false;

Vector4 getColorForGenome(std::string genome);
Vector4 getColorForGenome(std::string genome)
{
	const char *pGenome = genome.c_str();

    if (strlen(pGenome) == 1 && *pGenome == eInstructionPhotosynthesize)
        return Vector4(0,1,0,1);
    
    static std::map<std::string, Vector4> assignedColors;
    static std::vector<Vector4> colors;

    if (assignedColors.size() == 0)
        for (float r = .4f; r <= 1; r += .3f)
            for (float g = .4f; g <= 1; g += .3f)
                for (float b = .4f; b <= 1; b += .3f)
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
    mIsShowingHelp = mShowingInsertCritter = false;
    mViewScale = 1;
    Parameters::reset();
        
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

inline Vector3 getRandomSpherePoint()
{
    Vector3 v(UtilsRandom::getUnitRandom(), UtilsRandom::getUnitRandom(), UtilsRandom::getUnitRandom());
    v.normalize();
    
    return v;
}

bool gDecimateAgents = false;

/**
 * The thread function that runs the logic of the world, including all critter processing
 **/
void * Main :: threadFunction(void*)
{
    threadAlive = true;
    while (threadAlive)
    {
        ++numTurns;
        if (Parameters::speed == 0 || game.mShowingInsertCritter || game.mIsShowingHelp)
        {
            // stopped
            usleep(100);
        }
        else
        {
            pthread_mutex_lock( &mutex1 );
            gDecimateAgents = (world.step() > 50000);

            pthread_mutex_unlock( &mutex1 );
            unsigned int sleepTime = 50 * (10 - Parameters::speed);
            sleepTime *= sleepTime;
            
            if (sleepTime)
                usleep(sleepTime);
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

SegmentResourceMapping arraySegments[] = 
{
	iHelpPage1, "res/help/HelpPage1.png",
	iHelpPage2, "res/help/HelpPage2.png",
    iHelpPage1, "res/help/HelpPage1.png",
    iHelpPage2,"res/help/HelpPage2.png",
    iHelpPage3, "res/help/HelpPage3.png",
    iHelpPage4, "res/help/HelpPage4.png",
    iHelpPage5, "res/help/HelpPage5.png",
    iHelpClose, "res/help/close.png",

    iSpriteSphere, "res/sphere.png",
    iActiveSegment, "res/ActiveSegment.png",
    iGenericSegment, "res/segment.png",
    iSegmentFrame, "res/segmentFrame.png",
    iMoveArrow, "res/MoveArrow.png",

    iSegmentIf, "res/segment_exec_if.png",
    iSegmentIfNot, "res/segment_exec_no.png",
    iSegmentAlways, "res/segment_exec_always.png",

    eBarrier1, "res/barrier.png",
	eBarrier2, "res/barrier.png",
	eBarrier3, "res/barrier.png",
    
    eInstructionMoveAndEat, "res/segment_M.png",
    eInstructionMove, "res/segment_n.png",
    eInstructionTurnLeft, "res/segment_lt.png",
    eInstructionTurnRight, "res/segment_rt.png",
    eInstructionHardTurnLeft, "res/segment_[.png",
    eInstructionHardTurnRight, "res/segment_].png",
    eInstructionSleep, "res/segment_Z.png",
    
    eInstructionTestSeeFood, "res/segment_test_see_food.png",
    eInstructionTestNotSeeFood, "res/segment_test_not_see_food.png",
    eInstructionTestBlocked, "res/segment_test_blocked.png",
    eInstructionTestNotBlocked, "res/segment_test_not_blocked.png",
    eInstructionTestOccluded, "res/segment.png",
    
    eInstructionPhotosynthesize, "res/food.png",
    eInstructionFakePhotosynthesize, "res/segment_fakefood.png",

    eInstructionHyper, "res/segment_hyper.png",
    eInstructionTestPreyedOn, "res/segment_ifHungry.png"
};

/**
 Initialize the UI, resources and kick off the world's logic thread
 **/
void Main::initialize()
{
    mUIScale = this->getWidth() / 1024;
    
    mHelpPageRect = Rectangle(0, -140 * mUIScale, 1024 * mUIScale, 1024 * mUIScale);
    mHelpCloseRect =  Rectangle(950 * mUIScale, 12 * mUIScale, 40 * mUIScale, 40 * mUIScale);

    registerGesture(Gesture::GestureEvent(Gesture::GESTURE_PINCH));
    
    _font = Font::create((mUIScale == 1) ? "res/modata18.gpb" : "res/modata36.gpb");
    
    InstructionSet::reset();
    createUI();
    
	int i;
    for (i = 0; i < 255; i++)
        mSegmentBatch[i] = NULL;

	for (i = 0; i < sizeof(arraySegments)/sizeof(arraySegments[0]); i++)
	{
		mSegmentBatch[arraySegments[i].iSegmentType] = SpriteBatch::create(arraySegments[i].resSource);
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
    pthread_mutex_lock( &mutex1 );
    
    for (int i = 0; i < MAX_AGENTS; i++)
    {
        Agent & agent = world.mAgents[i];
        if (agent.mStatus == eAlive)
            world.killAgent(i);
    }
    
    string genome;
    bool bAllowMutation = true;
    int initialCount = 500;
    
//	genome += eInstructionMoveAndEat;
    genome += eInstructionPhotosynthesize;
    //bAllowMutation = false;

	if (false) {
		Agent *pAgent = world.createEmptyAgent();
		genome = eInstructionMove;
		genome += eInstructionPhotosynthesize;
		pAgent->initialize(getRandomSpherePoint(), genome.c_str(), bAllowMutation);
		pAgent->mEnergy = 100;
	}
	else
	if (false)
	{
		bAllowMutation = false;

		genome = eInstructionPhotosynthesize;
		int maxMoveCritters = 1;
		for (int i = 0; i < 7000; i++) {
			Agent *pAgent = world.createEmptyAgent();
			if (i < maxMoveCritters) {
				genome = eInstructionTestSeeFood;
				genome += eInstructionTurnLeft;
				genome += eInstructionMoveAndEat;
				genome += eInstructionMove;
			}
			else
				genome = eInstructionPhotosynthesize;
			pAgent->initialize(getRandomSpherePoint(), genome.c_str(), bAllowMutation);
			if (i < maxMoveCritters) {
				pAgent->mGenome[1].executeType = eNotIf;
				pAgent->mGenome[2].executeType = eIf;
			}
			//pAgent->mEnergy = Parameters::extraSpawnEnergyPerSegment/2;// (i == 4999) ? Parameters::extraSpawnEnergyPerSegment/2 :  Parameters::cycleEnergyCost * 2;
			// randomize the initial lifespan so they don't all die at once
			//pAgent->mLifespan = UtilsRandom::getRangeRandom(pAgent->mLifespan/2, pAgent->mLifespan*2);
			world.addAgentToWorld(pAgent);
		}
	}
	else
	{
		for (int i = 0; i < 500; i++)
		{
			Agent *pAgent = world.createEmptyAgent();
			pAgent->initialize(getRandomSpherePoint(), genome.c_str(), bAllowMutation);
			// randomize the initial lifespan so they don't all die at once
			pAgent->mLifespan = UtilsRandom::getRangeRandom(pAgent->mLifespan/2, pAgent->mLifespan*2);
			world.addAgentToWorld(pAgent);
		}
	}
    pthread_mutex_unlock( &mutex1 );
}

void Main :: resetParameters()
{
    Parameters::reset();
    setControlValues();
    updateControlLabels();
}

void Main :: insertCritter()
{
	if (mInsertGenome.length()) {
		pthread_mutex_lock( &mutex1 );
		for (int i = 0; i < 500; i++)
		{
			Agent *pAgent = world.createEmptyAgent(true);
			pAgent->initialize(getRandomSpherePoint(), mInsertGenome.c_str(), true);
			world.addAgentToWorld(pAgent);
		}
		pthread_mutex_unlock( &mutex1 );
	}
}

/**
 Turn on or off the barriers
 **/
void Main :: setBarriers(int type, bool bOn)
{
    pthread_mutex_lock( &mutex1 );
    static char barrierTypes[3] = { eBarrier1, eBarrier2, eBarrier3 };
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
            float barrierDistance = .05f;
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
    }
    pthread_mutex_unlock( &mutex1 );
}

void Main::finalize()
{
    threadAlive = false;
    void *status;
    pthread_join (mThread, &status);
    
    SAFE_RELEASE(_formAdvanced);
    SAFE_RELEASE(_formMain);
}

void Main::update(float elapsedTime)
{
    _formMain->update(elapsedTime);
    if (_showAdvanced->isChecked())
        _formAdvanced->update(elapsedTime);
    
    _formHelp->update(elapsedTime);
    
    if (mShowingInsertCritter)
    {
        _formInsertCritter->update(elapsedTime);
    }
}

/**
 Helper function and data for sampling the top "species", as identified by their genome
 **/
std::map<std::string, int> gMapSpeciesToCount;
vector<pair<std::string,int> > gTopSpecies;

bool compareTopSpeciesFunc(const pair<std::string,int> &p1, const pair<std::string,int> &p2);
bool compareTopSpeciesFunc(const pair<std::string,int> &p1, const pair<std::string,int> &p2)
{
    return p1.second > p2.second;
}


float gTurnsPerSecond = 0;

/**
 Render the world and the UI.
 */
void Main::render(float elapsedTime)
{
	try {
    // Clear the color and depth buffers
    clear(CLEAR_COLOR_DEPTH, Vector4::zero(), 1.0f, 0);
	int i;
	int reserveBatchCount = 200;
	for (i = 0; i < 255; i++)
		mSegmentBatchCount[i] = reserveBatchCount;

    for (int i = iActiveSegment; i < 255; i++)
        if (mSegmentBatch[i])
            mSegmentBatch[i]->start();
    
    // determine the size and position of the the world
    float renderSize = std::min(getWidth(), getHeight()) * .9;
    float offsetX = (getWidth() - renderSize) / 2;
    float offsetY = (getHeight() - renderSize) / 2;
    
    // draw the planet, slightly scaled down so the critters on the edges seem to have some height...
    mRenderSphereSize = renderSize * mViewScale * .995;
    mSphereOffsetX = (getWidth() - mRenderSphereSize) / 2;
    mSphereOffsetY = (getHeight() - mRenderSphereSize) / 2;
    
    _arcball.setBounds(mRenderSphereSize, mRenderSphereSize);
    
    mSegmentBatch[iSpriteSphere]->start();
    mSegmentBatch[iSpriteSphere]->draw(Rectangle(mSphereOffsetX, mSphereOffsetY, mRenderSphereSize, mRenderSphereSize), Rectangle(0,0,1024,1024));
    mSegmentBatch[iSpriteSphere]->finish();
    
    // once a second, determine the top critters
    bool sampleCritters = false;
    static float elapsedTimeSinceTally = 1000;
    static int startSampleTurns = 0;
    
    elapsedTimeSinceTally += elapsedTime;
    
    if (elapsedTimeSinceTally > 1000)
    {
        int numTurnsSinceTally = numTurns - startSampleTurns;
        startSampleTurns = numTurns;
        
        gTurnsPerSecond = ((float) numTurnsSinceTally) / (elapsedTimeSinceTally/1000);
        
        sampleCritters = true;
        gMapSpeciesToCount.clear();
        gTopSpecies.clear();
        
        elapsedTimeSinceTally = 0;
    }
    
    // draw all the visible agents in two passes, first drawing the segments, then drawing the
    // ornaments
    //pthread_mutex_lock( &mutex1 );
    for (int pass = 1; pass <= 2; pass++)
    {
        if (pass == 2 && mViewScale < 1.5)
            break;
        
        for (int i = 0; i <= world.mMaxLiveAgentIndex; i++)
        {
            Agent & agent = world.mAgents[i];
            if (agent.mStatus != eNonExistent)
            {
                if ((agent.mStatus == eAlive) && (agent.mSleep != -1))
                {
                    int count = gMapSpeciesToCount[agent.mGenome];
                    ++count;
                    gMapSpeciesToCount[agent.mGenome] = count;
                }
                
                for (int j = agent.mNumSegments-1; j >= 0; j--)
                {
                    SphereEntity *pEntity = &agent.mSegments[j];
                    
                    bool isPhotosynthesize = pEntity->mType == eInstructionPhotosynthesize;
                    
                    Vector3 pt = pEntity->mLocation;

                    mViewRotateMatrix.transformPoint(&pt);
                    Matrix viewScaleMatrix;
                    pt.x *= mViewScale;
                    pt.y *= mViewScale;
                    pt.z *= mViewScale;
                    
                    if (pt.z < 0)
                        continue; // cheap backface clipping
                    
                    // we use an orthogonal projection, but with some fakery to make it look more 3D
                    float cellSize = (Parameters::getMoveDistance() * pt.z * 400 + 3)  * mUIScale;
                    float x = ((pt.x) * (pt.z/5 + 1)) * .98;
                    float y = ((pt.y) * (pt.z/5 + 1)) * .98;
                    Rectangle dst = Rectangle(offsetX + renderSize * (x + 1) / 2 - cellSize / 2,
                                              offsetY + renderSize * (y + 1) / 2 - cellSize / 2,
                                              cellSize, cellSize);
                    
                    float alpha = 1;
                    if (agent.mStatus == eAlive)
                        alpha = (float)agent.mEnergy/(float)agent.getSpawnEnergy();
                    Vector4 color(1,1,1, alpha);
                    
                    int iBatch;
                    if (useGenomeColorMapping && agent.mStatus == eAlive && ! isPhotosynthesize)
                    {
                        iBatch = iGenericSegment;
                        color = getColorForGenome(agent.mGenome);
                        alpha = 1;
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
                        if ((agent.mNumSegments > 1) && (j == (agent.mActiveSegment) % agent.mNumSegments))
                        {
                            Rectangle activeSegmentRect = dst;
							Rectangle src = mSegmentSrcRect[iActiveSegment];
                            activeSegmentRect.inflate(cellSize / 4, cellSize / 4);
                            draw(iActiveSegment, activeSegmentRect, src, Vector4(1,1,1, 1));
                        }
                        
                        // draw the move arrow
                        if (agent.mIsMotile && (j == 0))
                        {
                            Vector3 moveVector3 = agent.mMoveVector;
                            mViewRotateMatrix.transformPoint(&moveVector3);
                            Vector2 moveVector(moveVector3.x, moveVector3.y);
                            moveVector.normalize();
                            float rotation = atan2(moveVector.y,moveVector.x);
                            
                            dst.inflate(cellSize*3/2, cellSize*3/2);
							Rectangle src = mSegmentSrcRect[iMoveArrow];
                            
                            Vector3 dstV(dst.x, dst.y,0);
                            Vector2 rotPoint(0.5f, 0.5f);
                            rotation += 45 * MATH_PI / 180;
                            draw(iMoveArrow, dstV, dst.width, dst.height, 0, 0, 1, 1, Vector4(1,1,1,1),
                                         rotPoint, rotation);
                            
                        }
                    }
                }
            }
        }
    }
    //pthread_mutex_unlock( &mutex1 );

	for (i = 0; i < 255; i++)
		mSegmentBatchCount[i] -= reserveBatchCount;

    // sort the sampled critters, if we're doing that...
    if (sampleCritters)
    {
        for (std::map<std::string, int>::iterator i = gMapSpeciesToCount.begin(); i != gMapSpeciesToCount.end(); i++)
            gTopSpecies.push_back(*i);
        
        sort(gTopSpecies.begin(), gTopSpecies.end(), compareTopSpeciesFunc);
    }
    
    _font->start();
    
    if (! mIsShowingHelp)
    {
        _font->drawText("Top Species", getWidth()-180 * mUIScale, 10 * mUIScale, Vector4(1,1,1,1));
        
        // draw the top species
        for (size_t i = 0; i < gTopSpecies.size(); i++)
        {
            if (i == 32)
                break;
            char buf[200];
            float x = getWidth()-50 * mUIScale;
            float y = mUIScale * (40 + 20 * i);
            
            sprintf(buf, "%d", gTopSpecies[i].second);
			std::string genome = gTopSpecies[i].first;
            const char *pGenome = genome.c_str();
            Vector4 drawColor(1,1,1,1);
            if (useGenomeColorMapping)
            {
                drawColor = getColorForGenome(gTopSpecies[i].first);
            }
            _font->drawText(buf, x, y, drawColor);
            
            x -= 40 * mUIScale;
            int len = strlen(pGenome);
            for (int j = (len - 1); j >= 0; j -= 2)
            {
                char ch = pGenome[j];
                Rectangle dst(x, y, 16 * mUIScale, 16 * mUIScale);
				Rectangle src = mSegmentSrcRect[ch];
                draw((int) ch, dst, src);

				if (ch != eInstructionPhotosynthesize && ch != eInstructionFakePhotosynthesize && ch != eInstructionPhotosynthesizeLess)
				{
					int iExecType = 0;
					SpriteBatch * pExecType = NULL;
					switch (pGenome[j-1]) {
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
						//_font->drawText("Y", dst.left(), dst.top(), Vector4(1,1,1,1));
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
            _font->drawText(buf, getWidth()-250 * mUIScale, getHeight() - 25 * mUIScale, Vector4(1,1,1,1));
        }
    }
    _font->finish();

    for (int i = iActiveSegment; i < 255; i++)
        if (mSegmentBatch[i])
            mSegmentBatch[i]->finish();
    
    // Draw the UI.
    _formMain->draw();
    if (_showAdvanced->isChecked())
        _formAdvanced->draw();
    
    _formHelp->draw();
    
    renderInsertCritter();
    renderHelp();
	}
	catch (...)
	{
		cout << "exception during render";
	}
}

/**
 This renders the "Insert Critter" dialog. Since there's no image button in gameplay, we just draw the segments
 on top of it. Crude, but it works.
 */
void Main::renderInsertCritter()
{
    if (mShowingInsertCritter)
    {
        for (int i = iSpriteSphere; i < 255; i++)
            if (mSegmentBatch[i])
                mSegmentBatch[i]->start();
        _formInsertCritter->draw();
        for (std::vector<Button*>::iterator i = mInsertInstructionButtons.begin(); i != mInsertInstructionButtons.end(); i++)
        {
            Button *pButton = *i;
            SpriteBatch * pBatch = mSegmentBatch[pButton->getId()[0]];
			if (! pBatch)
				pBatch = mSegmentBatch[iGenericSegment];
            Rectangle src(0,0,
                          pBatch->getSampler()->getTexture()->getWidth(), pBatch->getSampler()->getTexture()->getHeight());
            
            Rectangle dst = pButton->getBounds();
            dst.x += _formInsertCritter->getX() + 20 * mUIScale;
            dst.y += _formInsertCritter->getY() + 18 * mUIScale;
            dst.inflate(-6 * mUIScale,-6 * mUIScale);
            pBatch->draw(dst, src);
        }
        
        float x = mInsertCritterGenome->getX() + _formInsertCritter->getX() + 24 * mUIScale;
        float y = mInsertCritterGenome->getY() + _formInsertCritter->getY() + 20 * mUIScale;
        float w = 5 * mUIScale + mInsertCritterGenome->getWidth() / MAX_GENOME_LENGTH;
        for (string::iterator i = mInsertGenome.begin(); i != mInsertGenome.end(); i++)
        {
            SpriteBatch * pBatch = mSegmentBatch[*i];
            Rectangle src(0,0,
                          pBatch->getSampler()->getTexture()->getWidth(), pBatch->getSampler()->getTexture()->getHeight());
            
            Rectangle dst(x, y, w, w);
            dst.inflate(-6 * mUIScale, -6 * mUIScale);
            pBatch->draw(dst, src);
            
            x += w - 6 * mUIScale;
        }
        
        for (int i = iSpriteSphere; i < 255; i++)
            if (mSegmentBatch[i])
                mSegmentBatch[i]->finish();
    }
}

/**
 Render the help tabs, with the inactive tabs shown in gray
 **/
void Main::renderHelp()
{
    if (mIsShowingHelp)
    {
        for (int pass = 1; pass <= 2; pass++)
        {
            for (int i = 0; i < 5; i++)
            {
                SpriteBatch * pHelpPage = mSegmentBatch[i];
                Rectangle src(0,0,1024,1024);
                
                if ((pass == 1) && (i != mHelpPageIndex))
                {
                    pHelpPage->start();
                    pHelpPage->draw(mHelpPageRect, src, Vector4(.75,.75,.75,1));
                    pHelpPage->finish();
                }
                else if ((pass == 2) && (i == mHelpPageIndex))
                {
                    pHelpPage->start();
                    pHelpPage->draw(mHelpPageRect, src);
                    pHelpPage->finish();
                    break;
                }
            }
        }
        SpriteBatch * pClose = mSegmentBatch[iHelpClose];
        Rectangle src(0,0, 128, 128);
        pClose->start();
        pClose->draw(mHelpCloseRect, src);
        pClose->finish();
    }
}


void Main::keyEvent(Keyboard::KeyEvent evt, int key)
{
    if (evt == Keyboard::KEY_PRESS)
    {
        switch (key)
        {
            case Keyboard::KEY_ESCAPE:
                exit();
                break;
        }
    }
}

static float gTouchStartViewScale;

void Main::touchEvent(Touch::TouchEvent evt, int x, int y, unsigned int contactIndex)
{
    // another bit of quick & dirtiness...if showing help, handle the touch events.
    // hard coded values are bad. Banging out an iOS app on the way to work is good. ;)
    if (mIsShowingHelp)
    {
        if (mHelpCloseRect.contains(x, y))
        {
            mIsShowingHelp = false;
            _formHelp->setPosition(_formHelp->getX(), -_formHelp->getY());
        }
        else
        {
            x -= mHelpPageRect.left();
            y -= mHelpPageRect.top();
            
            if ((y >= 835) && (y <= 890) && (x >= 30) && (x <= 990))
            {
                if ((x >= 30) && (x <= 285))
                    mHelpPageIndex = 0;
                else if (x <= 475)
                    mHelpPageIndex = 1;
                else if (x <= 605)
                    mHelpPageIndex = 2;
                else if (x <= 816)
                    mHelpPageIndex = 3;
                else
                    mHelpPageIndex = 4;
            }
        }
        
        return;
    }
    
    static bool down = false;
    
    static Quaternion initialRotation;
    static Quaternion lastDragRotation;
    
    Vector2 sphereDisplayPoint(x - mSphereOffsetX, mRenderSphereSize - (y - mSphereOffsetY));
    switch (evt)
    {
        case Touch::TOUCH_PRESS:
            if (! down)
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


void Main::createUI()
{
    // Create main form
    _formMain = createForm(335, 340);
    _formMain->setPosition(4 * mUIScale, 8 * mUIScale);
    
    _cellSizeSlider = createSliderControl(_formMain, "cellSize", "Cell size:", 1, 10, 1);
    _speedSlider = createSliderControl(_formMain, "speed", "Speed:", 0, 10, 1);
    _mutationSlider = createSliderControl(_formMain, "mutation", "Mutation:", 0, 10);
    createSpacer(_formMain, 5);
    _barriers1 = createCheckboxControl(_formMain, "Barriers #1");
    _barriers2 = createCheckboxControl(_formMain, "Barriers #2");
    
    _showAdvanced = createCheckboxControl(_formMain, "Show advanced settings");
    
    createSpacer(_formMain, 15);
    _resetWorldButton = createButton(_formMain, "Reset world");
    createSpacer(_formMain, 4);
    _insertButton = createButton(_formMain, "Insert...");
    
    // Create advanced form, optionally shown
    _formAdvanced = createForm(330, 380);
    _formAdvanced->setPosition(_formAdvanced->getX() + 4 * mUIScale, 365 * mUIScale);
    
    createControlHeader(_formAdvanced, "Energy cost / gain");
    // _cycleEnergyCostSlider = createSliderControl(_formAdvanced, "cycleEnergyCost", "Cycle:", .5, 2);
    _photoSynthesizeEnergyGainSlider = createSliderControl(_formAdvanced, "photoSynthesizeEnergyGain", "Photosynthesis:", 1, 2);
    _moveEnergyCostSlider = createSliderControl(_formAdvanced, "moveEnergyCost", "Move:", 0, 15);
    _moveAndEatEnergyCostSlider = createSliderControl(_formAdvanced, "moveAndEatEnergyCost", "Move & eat:", 0, 30);
    
    //createControlHeader(_formAdvanced,"Energy to spawn");
    _extraSpawnEnergyPerSegmentSlider = createSliderControl(_formAdvanced,"extraSpawnEnergyPerSegment", "Per cell:", 50, 2000);
    
    //createControlHeader(_formAdvanced,"Other");
    _lookDistanceSlider = createSliderControl(_formAdvanced,"lookDistance", "Vision range:", 1, 30);
    //createSpacer(_formAdvanced, 15);
    //_resetParametersButton = createButton(_formAdvanced, "Reset settings");
    
    _extraCyclesForMoveSlider = createSliderControl(_formAdvanced,"extraCyclesForMove", "Move delay:", 0, 20);
    _biteStrengthSlider = createSliderControl(_formAdvanced,"biteStrength", "Bite strength:", 0.1, 5);
	_digestionEfficiencySlider = createSliderControl(_formAdvanced, "digestionEfficiency", "Digestion:", 0.1, 1);

    _allowSelfOverlap = createCheckboxControl(_formAdvanced, "Allow self overlap");

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
    
    _helpButton = createButton(_formHelp, "What is this?");
    _colorCodeSpecies = createCheckboxControl(_formHelp, "Color-code species");
    _colorCodeSpecies->setFontSize(30 * mUIScale);
    _colorCodeSpecies->setTextAlignment(Font::ALIGN_VCENTER);
    
    // create insert critter form
    _formInsertCritter = createForm(900, 310, false);
    _formInsertCritter->setPosition((getWidth() - _formInsertCritter->getWidth()) / 2,
                                    (getHeight() - _formInsertCritter->getHeight()) / 2);
    
    _formInsertCritter->setPosition(_formInsertCritter->getX(), -_formInsertCritter->getY());
    
    
    mInsertOK = createButton(_formInsertCritter, "Insert x 500", "", Vector2(560,230), Vector2(180, 40));
    mInsertCancel = createButton(_formInsertCritter, "Cancel", "", Vector2(740,230), Vector2(120, 40));
    mInsertCritterGenome = createContainer(_formInsertCritter, true, Vector2(20,20), Vector2(720, 56));
    mInsertClear = createButton(_formInsertCritter, "Clear", "", Vector2(755,28), Vector2(80, 40));
    
    set<char> availableInstructions = InstructionSet::getAllAvailableInstructions();
    
    float x = 20;
    float y = 80;
    for (set<char>::iterator i = availableInstructions.begin(); i != availableInstructions.end(); i++)
    {
        string s;
        s = *i;
        
        Button * pInstruction = createButton(_formInsertCritter, " ", s.c_str(), Vector2(x, y), Vector2(60, 60));
        mInsertInstructionButtons.push_back(pInstruction);
        x += 60;
    }
    
    createControlHeader(_formInsertCritter, "Click on the instructions above to build your genome, then press 'Insert x 500'.\nClick on 'What is this?' for a full description of each instruction.",
                        Vector2(40,140), Vector2(900, 60), Vector4(1,1,1,1));

    setControlValues();
    updateControlLabels();
}

void Main::controlEvent(Control* control, EventType evt)
{
    switch(evt)
    {
        default:
            break;
            
        case Listener::CLICK:
            if (control == _resetWorldButton)
                resetWorld();
            else if (control == _resetParametersButton)
                resetParameters();
            else if (control == _helpButton)
            {
                mIsShowingHelp = true;
                _formHelp->setPosition(_formHelp->getX(), -_formHelp->getY());
                mHelpPageIndex = 0;
            }
            else if (control == _insertButton)
            {
                mInsertGenome = "";
                mShowingInsertCritter = true;
                _formInsertCritter->setPosition(_formInsertCritter->getX(), -_formInsertCritter->getY());
                mInsertOK->setPosition(mInsertOK->getX(), -mInsertOK->getY());
            } else if (control == mInsertClear)
            {
                if (mInsertGenome.length())
                {
                    mInsertGenome = mInsertGenome.substr(0, mInsertGenome.length()-1);
                    if (mInsertGenome.length() == 0)
                        mInsertOK->setPosition(mInsertOK->getX(), -mInsertOK->getY());
                }
            } else if (control == mInsertCancel)
            {
                mShowingInsertCritter = false;
                _formInsertCritter->setPosition(_formInsertCritter->getX(), -_formInsertCritter->getY());
            } else if (control == mInsertOK)
            {
                insertCritter();
                mShowingInsertCritter = false;
                _formInsertCritter->setPosition(_formInsertCritter->getX(), -_formInsertCritter->getY());
            } else if (find(mInsertInstructionButtons.begin(), mInsertInstructionButtons.end(),control) !=
                       mInsertInstructionButtons.end())
            {
                if (mInsertGenome.length() < MAX_GENOME_LENGTH)
                    mInsertGenome += control->getId();
                if (mInsertGenome.length() == 1)
                    mInsertOK->setPosition(mInsertOK->getX(), -mInsertOK->getY());
            }
            break;
            
        case Listener::VALUE_CHANGED:
            if (control == _colorCodeSpecies)
                useGenomeColorMapping = _colorCodeSpecies->isChecked();
            else if (control == _speedSlider)
                Parameters::speed = _speedSlider->getValue();
            else if (control == _mutationSlider)
                Parameters::mutationPercent = _mutationSlider->getValue();
            else if (control == _cellSizeSlider)
                Parameters::cellSize = _cellSizeSlider->getValue();
//            else if (control == _cycleEnergyCostSlider)
//                Parameters::cycleEnergyCost = _cycleEnergyCostSlider->getValue();
            else if (control == _photoSynthesizeEnergyGainSlider)
                Parameters::photoSynthesizeEnergyGain = _photoSynthesizeEnergyGainSlider->getValue();
            else if (control == _moveEnergyCostSlider)
                Parameters::moveEnergyCost = _moveEnergyCostSlider->getValue();
            else if (control == _moveAndEatEnergyCostSlider)
                Parameters::moveAndEatEnergyCost = _moveAndEatEnergyCostSlider->getValue();
            else if (control == _baseSpawnEnergySlider)
                Parameters::baseSpawnEnergy = _baseSpawnEnergySlider->getValue();
            else if (control == _extraSpawnEnergyPerSegmentSlider)
                Parameters::extraSpawnEnergyPerSegment = _extraSpawnEnergyPerSegmentSlider->getValue();
            else if (control == _lookDistanceSlider)
                Parameters::lookDistance = _lookDistanceSlider->getValue();
            else if (control == _allowSelfOverlap)
                Parameters::allowSelfOverlap = _allowSelfOverlap->isChecked();
			else if (control == _extraCyclesForMoveSlider)
				Parameters::extraCyclesForMove = _extraCyclesForMoveSlider->getValue();
			else if (control == _biteStrengthSlider)
				Parameters::biteStrength = _biteStrengthSlider->getValue();
			else if (control == _digestionEfficiencySlider)
				Parameters::digestionEfficiency = _digestionEfficiencySlider->getValue();
            else if (control == _barriers1)
                setBarriers(0, _barriers1->isChecked());
            else if (control == _barriers2)
                setBarriers(1, _barriers2->isChecked());
            else if (control == _barriers3)
                setBarriers(2, _barriers3->isChecked());
            updateControlLabels();
            break;
    }
}


void Main :: updateControlLabel(std::string parameterId, const char *pFormat, ...)
{
    Label * pLabel = (Label*)_formMain->getControl((parameterId + "Label").c_str());
    if (! pLabel)
        pLabel = (Label*)_formAdvanced->getControl((parameterId + "Label").c_str());
    
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
    
    updateControlLabel("speed", "%d", Parameters::speed);
    updateControlLabel("mutation", "%d%%", Parameters::mutationPercent);
    updateControlLabel("cellSize", "%d", (int) Parameters::cellSize);
//    updateControlLabel("cycleEnergyCost", "-%.1f", Parameters::cycleEnergyCost);
    updateControlLabel("photoSynthesizeEnergyGain", "+%.1f", Parameters::photoSynthesizeEnergyGain);
    updateControlLabel("moveEnergyCost", "-%.1f", Parameters::moveEnergyCost);
    updateControlLabel("moveAndEatEnergyCost", "-%.1f", Parameters::moveAndEatEnergyCost);
    updateControlLabel("baseSpawnEnergy", "%d", (int) Parameters::baseSpawnEnergy);
    updateControlLabel("extraSpawnEnergyPerSegment", "%d", (int) Parameters::extraSpawnEnergyPerSegment);
    updateControlLabel("lookDistance", "%d", Parameters::lookDistance);
    updateControlLabel("extraCyclesForMove", "%d", Parameters::extraCyclesForMove);
	updateControlLabel("biteStrength", "%d%%", (int)(Parameters::biteStrength * 100));
	updateControlLabel("digestionEfficiency", "%d%%", (int)(Parameters::digestionEfficiency * 100));
	
}

void Main::setControlValues()
{
    _speedSlider->setValue(Parameters::speed);
    _mutationSlider->setValue(Parameters::mutationPercent);
    _cellSizeSlider->setValue(Parameters::cellSize);
    _photoSynthesizeEnergyGainSlider->setValue(Parameters::photoSynthesizeEnergyGain);
    _moveEnergyCostSlider->setValue(Parameters::moveEnergyCost);
    _moveAndEatEnergyCostSlider->setValue(Parameters::moveAndEatEnergyCost);
//    _baseSpawnEnergySlider->setValue(Parameters::baseSpawnEnergy);
    _extraSpawnEnergyPerSegmentSlider->setValue(Parameters::extraSpawnEnergyPerSegment);
    _lookDistanceSlider->setValue(Parameters::lookDistance);
    _allowSelfOverlap->setChecked(Parameters::allowSelfOverlap);

	_extraCyclesForMoveSlider->setValue(Parameters::extraCyclesForMove);
	_biteStrengthSlider->setValue(Parameters::biteStrength);
	_digestionEfficiencySlider->setValue(Parameters::digestionEfficiency);
    this->_formAdvanced->update(1);
}

/**
 * Methods for programmatically creating the UI
 */
#define LABEL_WIDTH 130
#define SLIDER_WIDTH 140

Form * Main :: createForm(float width, float height, bool isLayoutVertical)
{
    Form * result = Form::create(isLayoutVertical ? "res/layoutVertical.form" : "res/layoutAbsolute.form");
    
    Container *pMainContainer = (Container*)result->getControl("main");
    pMainContainer->setSkinColor(Vector4(0,0,0,1));
    
    result->setSize(width * mUIScale, height * mUIScale);
    pMainContainer->setSize(width * mUIScale, (height - 20) * mUIScale);
    pMainContainer->setPosition(pMainContainer->getX() * mUIScale, pMainContainer->getY() * mUIScale);
    {
        const Theme::Margin& m = pMainContainer->getMargin();
        pMainContainer->setMargin(m.top * mUIScale, m.bottom * mUIScale, m.left * mUIScale, m.right * mUIScale);
        const Theme::Border& b = pMainContainer->getBorder();
        pMainContainer->setBorder(b.top * mUIScale, b.bottom * mUIScale, b.left * mUIScale, b.right * mUIScale);
    }
    {
        const Theme::Margin& m = result->getMargin();
        result->setMargin(m.top * mUIScale, m.bottom * mUIScale, m.left * mUIScale, m.right * mUIScale);
        const Theme::Border& b = result->getBorder();
        result->setBorder(b.top * mUIScale, b.bottom * mUIScale, b.left * mUIScale, b.right * mUIScale);
    }
    return result;
}

void Main :: createControlHeader(Form *form, std::string text, Vector2 pos, Vector2 size, Vector4 textColor)
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
    pContainer->setSize(350 * mUIScale, 35 * mUIScale);
    
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
    pCheckbox->setFontSize(pCheckbox->getFontSize() * mUIScale);
    pCheckbox->setImageSize(20 * mUIScale, 20 * mUIScale);
    
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
        size.x = 285;
        size.y = 40;
    }
    
    Container *pMainContainer = (Container*)form->getControl("main");
    
    Button *pButton = Button::create(id, pBasic);
    pButton->setZIndex(pMainContainer->getControls().size());
    pMainContainer->addControl(pButton);
    pButton->setText(label.c_str());
    pButton->setSize(size.x * mUIScale, size.y * mUIScale);
    
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

	if (mSegmentBatchCount[iBatch] >= 60000)
		return NULL;
	++mSegmentBatchCount[iBatch];
	return pResult;
}

void Main :: draw(int iBatch, const Rectangle& dst, const Rectangle& src, const Vector4& color)
{
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

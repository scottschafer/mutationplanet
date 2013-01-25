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
#include <pthread.h>
#include "Parameters.h"

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
    iSegmentFrame,
    iMoveArrow
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

Main::Main()
{
    mIsShowingHelp = mShowingInsertCritter = false;
    mViewScale = 1;
    Parameters::reset();
    
    mHelpPageRect = Rectangle(0, -140, 1024, 1024);
    mHelpCloseRect =  Rectangle(950, 12, 40, 40);
    
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
            world.step();
            pthread_mutex_unlock( &mutex1 );
            uint sleepTime = 10 * (10 - Parameters::speed);
            sleepTime *= sleepTime * sleepTime;
            
            if (sleepTime)
                usleep(sleepTime);
        }
    }
    return NULL;
}

/**
 Initialize the UI, resources and kick off the world's logic thread
 **/
void Main::initialize()
{
    // Display the gameplay splash screen for at least 1 second.
    displayScreen(this, &Main::drawSplash, NULL, 2000L);
    
    registerGesture(Gesture::GestureEvent(Gesture::GESTURE_PINCH));
    
    mViewRotateMatrix.identity();
    
    _font = Font::create("res/modata18.gpb");
    
    InstructionSet::reset();
    createUI();
    
    for (int i = 0; i < 255; i++)
        mSegmentBatch[i] = NULL;
    
    mSegmentBatch[iHelpPage1] = SpriteBatch::create("res/help/HelpPage1.png");
    mSegmentBatch[iHelpPage2] = SpriteBatch::create("res/help/HelpPage2.png");
    mSegmentBatch[iHelpPage3] = SpriteBatch::create("res/help/HelpPage3.png");
    mSegmentBatch[iHelpPage4] = SpriteBatch::create("res/help/HelpPage4.png");
    mSegmentBatch[iHelpPage5] = SpriteBatch::create("res/help/HelpPage5.png");
    mSegmentBatch[iHelpClose] = SpriteBatch::create("res/help/close.png");
    
    mSegmentBatch[iSpriteSphere] = SpriteBatch::create("res/sphere.png");
    mSegmentBatch[iActiveSegment] = SpriteBatch::create("res/ActiveSegment.png");
    mSegmentBatch[iSegmentFrame] = SpriteBatch::create("res/segmentFrame.png");
    mSegmentBatch[iMoveArrow] = SpriteBatch::create("res/MoveArrow.png");
    
    mSegmentBatch[eBarrier1] = mSegmentBatch[eBarrier2] = mSegmentBatch[eBarrier3] = SpriteBatch::create("res/barrier.png");
    
    mSegmentBatch[eInstructionMoveAndEat] = SpriteBatch::create("res/segment_M.png");
    mSegmentBatch[eInstructionMove] = SpriteBatch::create("res/segment_n.png");
    mSegmentBatch[eInstructionTurnLeft] = SpriteBatch::create("res/segment_<.png");
    mSegmentBatch[eInstructionTurnRight] = SpriteBatch::create("res/segment_>.png");
    mSegmentBatch[eInstructionHardTurnLeft] = SpriteBatch::create("res/segment_[.png");
    mSegmentBatch[eInstructionHardTurnRight] = SpriteBatch::create("res/segment_].png");
    mSegmentBatch[eInstructionSleep] = SpriteBatch::create("res/segment_Z.png");
    
    mSegmentBatch[eInstructionTestSeeFood] = SpriteBatch::create("res/segment_test_see_food.png");
    mSegmentBatch[eInstructionTestNotSeeFood] = SpriteBatch::create("res/segment_test_not_see_food.png");
    mSegmentBatch[eInstructionTestBlocked] = SpriteBatch::create("res/segment_test_blocked.png");
    mSegmentBatch[eInstructionTestNotBlocked] = SpriteBatch::create("res/segment_test_not_blocked.png");
    
    mSegmentBatch[eInstructionPhotosynthesize] = SpriteBatch::create("res/food.png");
    mSegmentBatch[eInstructionFakePhotosynthesize] = SpriteBatch::create("res/segment_fakefood.png");
    
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
    
    genome += eInstructionPhotosynthesize;
    
    for (int i = 0; i < 500; i++)
    {
        Agent *pAgent = world.createEmptyAgent();
        pAgent->initialize(getRandomSpherePoint(), genome.c_str(), bAllowMutation);
        // randomize the initial lifespan so they don't all die at once
        pAgent->mLifespan = UtilsRandom::getRangeRandom(pAgent->mLifespan/2, pAgent->mLifespan*2);
        world.addAgentToWorld(pAgent);
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
    pthread_mutex_lock( &mutex1 );
    for (int i = 0; i < 500; i++)
    {
        Agent *pAgent = world.createEmptyAgent(true);
        pAgent->initialize(getRandomSpherePoint(), mInsertGenome.c_str(), true);
        world.addAgentToWorld(pAgent);
    }
    pthread_mutex_unlock( &mutex1 );
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
            float barrierDistance = .008;
            for (float a = -MATH_PI * .95; a < MATH_PI * .95; a += barrierDistance)
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
            float barrierDistance = .05;
            for (float a = -MATH_PI * .98; a < MATH_PI * .98; a += barrierDistance)
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
std::map<string, int> gMapSpeciesToCount;
vector<pair<string,int> > gTopSpecies;

bool compareTopSpeciesFunc(const pair<string,int> &p1, const pair<string,int> &p2);
bool compareTopSpeciesFunc(const pair<string,int> &p1, const pair<string,int> &p2)
{
    return p1.second > p2.second;
}


float gTurnsPerSecond = 0;

/**
 Render the world and the UI.
 */
void Main::render(float elapsedTime)
{
    // Clear the color and depth buffers
    clear(CLEAR_COLOR_DEPTH, Vector4::zero(), 1.0f, 0);
    
    for (int i = iSpriteSphere; i < 255; i++)
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
    
    mSegmentBatch[iSpriteSphere]->draw(Rectangle(mSphereOffsetX, mSphereOffsetY, mRenderSphereSize, mRenderSphereSize), Rectangle(0,0,1024,1024));
    
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
    for (int pass = 1; pass <= 2; pass++)
    {
        if (pass == 2 && mViewScale < 2)
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
                    Vector3 pt = pEntity->mLocation;
                    mViewRotateMatrix.transformPoint(&pt);
                    Matrix viewScaleMatrix;
                    viewScaleMatrix.scale(mViewScale);
                    viewScaleMatrix.transformPoint(&pt);
                    
                    if (pt.z < 0)
                        continue; // cheap backface clipping
                    
                    // we use an orthogonal projection, but with some fakery to make it look more 3D
                    float cellSize = Parameters::getMoveDistance() * pt.z * 480;
                    float x = ((pt.x) * (pt.z/5 + 1)) * .98;
                    float y = ((pt.y) * (pt.z/5 + 1)) * .98;
                    Rectangle dst = Rectangle(offsetX + renderSize * (x + 1) / 2 - cellSize / 2,
                                              offsetY + renderSize * (y + 1) / 2 - cellSize / 2,
                                              cellSize, cellSize);
                    
                    SpriteBatch * pBatch = mSegmentBatch[pEntity->mType];
                    
                    Rectangle src = Rectangle(0,0,
                                              pBatch->getSampler()->getTexture()->getWidth(), pBatch->getSampler()->getTexture()->getHeight());
                    
                    float alpha = 1;
                    if (agent.mStatus == eAlive)
                        alpha = (float)agent.mEnergy/(float)agent.getSpawnEnergy();
                    
                    if (pass == 1)
                        pBatch->draw(dst, src, Vector4(1,1,1, alpha));
                    
                    // if we're zoomed in, show an indicator around the active cell
                    if ((pass == 2) && (agent.mNumSegments > 1))
                    {
                        if (j == (agent.mActiveSegment /* + agent.mNumSegments - 1 */) % agent.mNumSegments)
                        {
                            pBatch = mSegmentBatch[iActiveSegment];
                            src = Rectangle(0,0,
                                            pBatch->getSampler()->getTexture()->getWidth(), pBatch->getSampler()->getTexture()->getHeight());
                            
                            Rectangle activeSegmentRect = dst;
                            activeSegmentRect.inflate(cellSize / 4, cellSize / 4);
                            pBatch->draw(activeSegmentRect, src, Vector4(1,1,1, 1));
                        }
                        
                        // draw the move arrow
                        if (agent.mIsMotile && (j == 0))
                        {
                            Vector3 moveVector3 = agent.mMoveVector;
                            mViewRotateMatrix.transformPoint(&moveVector3);
                            Vector2 moveVector(moveVector3.x, moveVector3.y);
                            moveVector.normalize();
                            float rotation = atan2(moveVector.y,moveVector.x);
                            
                            pBatch = mSegmentBatch[iMoveArrow];
                            dst.inflate(cellSize*3/2, cellSize*3/2);
                            src = Rectangle(0,0,
                                            pBatch->getSampler()->getTexture()->getWidth(), pBatch->getSampler()->getTexture()->getHeight());
                            
                            
                            Vector3 dstV(dst.x, dst.y,0);
                            Vector2 rotPoint(0.5f, 0.5f);
                            rotation += 45 * MATH_PI / 180;
                            pBatch->draw(dstV, dst.width, dst.height, 0, 0, 1, 1, Vector4(1,1,1,1),
                                         rotPoint, rotation);
                            
                        }
                    }
                }
            }
        }
    }
    
    // sort the sampled critters, if we're doing that...
    if (sampleCritters)
    {
        for (std::map<string, int>::iterator i = gMapSpeciesToCount.begin(); i != gMapSpeciesToCount.end(); i++)
            gTopSpecies.push_back(*i);
        
        sort(gTopSpecies.begin(), gTopSpecies.end(), compareTopSpeciesFunc);
    }
    
    _font->start();
    
    if (! mIsShowingHelp)
    {
        _font->drawText("Top Species", getWidth()-180, 10, Vector4(1,1,1,1));
        
        // draw the top species
        for (int i = 0; i < gTopSpecies.size(); i++)
        {
            if (i == 32)
                break;
            char buf[200];
            float x = getWidth()-50;
            float y = 40 + 20 * i;
            
            sprintf(buf, "%d", gTopSpecies[i].second);
            _font->drawText(buf, x, y, Vector4(1,1,1,1));
            
            x -= 40;
            const char *pGenome = gTopSpecies[i].first.c_str();
            int len = strlen(pGenome);
            for (int j = (len - 1); j >= 0; j--)
            {
                char ch = pGenome[j];
                SpriteBatch * pBatch = mSegmentBatch[ch];
                Rectangle src(0,0,
                              pBatch->getSampler()->getTexture()->getWidth(), pBatch->getSampler()->getTexture()->getHeight());
                Rectangle dst(x, y, 16, 16);
                pBatch->draw(dst, src);
                x -= 18;
            }
        }
        
        // show the turns per second
        totalElapsedTime += elapsedTime;
        
        if (gTurnsPerSecond > 0)
        {
            char buf[200];
            sprintf(buf, "Turns/sec: %d", (int) gTurnsPerSecond);
            _font->drawText(buf,  getWidth()-200, getHeight() - 25, Vector4(1,1,1,1));
        }
    }
    
    for (int i = iSpriteSphere; i < 255; i++)
        if (mSegmentBatch[i])
            mSegmentBatch[i]->finish();
    
    // Draw the UI.
    _formMain->draw();
    if (_showAdvanced->isChecked())
        _formAdvanced->draw();
    
    _formHelp->draw();
    
    renderInsertCritter();
    renderHelp();
    
    _font->finish();
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
            Rectangle src(0,0,
                          pBatch->getSampler()->getTexture()->getWidth(), pBatch->getSampler()->getTexture()->getHeight());
            
            Rectangle dst = pButton->getBounds();
            dst.x += _formInsertCritter->getX() + 20;
            dst.y += _formInsertCritter->getY() + 18;
            dst.inflate(-6,-6);
            pBatch->draw(dst, src);
        }
        
        float x = mInsertCritterGenome->getX() + _formInsertCritter->getX() + 24;
        float y = mInsertCritterGenome->getY() + _formInsertCritter->getY() + 20;
        float w = 5 + mInsertCritterGenome->getWidth() / MAX_GENOME_LENGTH;
        for (string::iterator i = mInsertGenome.begin(); i != mInsertGenome.end(); i++)
        {
            SpriteBatch * pBatch = mSegmentBatch[*i];
            Rectangle src(0,0,
                          pBatch->getSampler()->getTexture()->getWidth(), pBatch->getSampler()->getTexture()->getHeight());
            
            Rectangle dst(x, y, w, w);
            dst.inflate(-6, -6);
            pBatch->draw(dst, src);
            
            x += w - 6;
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
    static int lastX, lastY;
    
    static Matrix rotateMatrix;
    static Matrix initialViewMatrix;
    
    Vector2 sphereDisplayPoint(x - mSphereOffsetX, mRenderSphereSize - (y - mSphereOffsetY));
    switch (evt)
    {
        case Touch::TOUCH_PRESS:
            gTouchStartViewScale = mViewScale;
            _arcball.click(sphereDisplayPoint);
            rotateMatrix.identity();
            initialViewMatrix = mViewRotateMatrix;
            break;
            
        case Touch::TOUCH_RELEASE:
            down = false;
            break;
            
        case Touch::TOUCH_MOVE: {
            gameplay::Quaternion q;
            _arcball.drag(sphereDisplayPoint, &q);
            Matrix::createRotation(q, &rotateMatrix);
            rotateMatrix.multiply(initialViewMatrix);
            mViewRotateMatrix = rotateMatrix;
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
    _formMain = createForm(320, 330);
    _formMain->setPosition(4, 8);
    
    _cellSizeSlider = createSliderControl(_formMain, "cellSize", "Cell size:", 1, 10, 1);
    _speedSlider = createSliderControl(_formMain, "speed", "Speed:", 0, 10, 1);
    _mutationSlider = createSliderControl(_formMain, "mutation", "Mutation:", 0, 10);
    createSpacer(_formMain, 15);
    _barriers1 = createCheckboxControl(_formMain, "Barriers #1");
    _barriers2 = createCheckboxControl(_formMain, "Barriers #2");
    
    _showAdvanced = createCheckboxControl(_formMain, "Show advanced settings");
    
    createSpacer(_formMain, 15);
    _resetWorldButton = createButton(_formMain, "Reset world");
    createSpacer(_formMain, 4);
    _insertButton = createButton(_formMain, "Insert...");
    
    // Create advanced form, optionally shown
    _formAdvanced = createForm(320, 375);
    _formAdvanced->setPosition(_formAdvanced->getX() + 4, 365);
    
    createControlHeader(_formAdvanced, "Energy cost / gain");
    // _cycleEnergyCostSlider = createSliderControl(_formAdvanced, "cycleEnergyCost", "Cycle:", .5, 2);
    _photoSynthesizeEnergyGainSlider = createSliderControl(_formAdvanced, "photoSynthesizeEnergyGain", "Photosynthesis:", 1, 20);
    _moveEnergyCostSlider = createSliderControl(_formAdvanced, "moveEnergyCost", "Move:", 0, 15);
    _moveAndEatEnergyCostSlider = createSliderControl(_formAdvanced, "moveAndEatEnergyCost", "Move & eat:", 0, 15);
    
    createControlHeader(_formAdvanced,"Energy to spawn");
//    _baseSpawnEnergySlider = createSliderControl(_formAdvanced,"baseSpawnEnergy", "Base:", 50, 500);
    _extraSpawnEnergyPerSegmentSlider = createSliderControl(_formAdvanced,"extraSpawnEnergyPerSegment", "Per cell:", 50, 500);
    
    createControlHeader(_formAdvanced,"Other");
    _lookDistanceSlider = createSliderControl(_formAdvanced,"lookDistance", "Vision range:", 1, 15);
    _allowSelfOverlap = createCheckboxControl(_formAdvanced, "Allow self overlap");
    createSpacer(_formAdvanced, 15);
    _resetParametersButton = createButton(_formAdvanced, "Reset settings");
    
    _formMain->setConsumeInputEvents(false);
    _formAdvanced->setConsumeInputEvents(false);
    
    _formHelp = Form::create("res/editor.form");
    _formHelp->setPosition(720, 700);
    Theme::Style * pNoBorder = _formHelp->getTheme()->getStyle("noBorder");
    _formHelp->setStyle(pNoBorder);
    _formHelp->getControl("main")->setStyle(pNoBorder);
    
    _helpButton = createButton(_formHelp, "What is this?");
    
    // create insert critter form
    _formInsertCritter = createForm(900, 310, false);
    _formInsertCritter->setPosition((1024 - _formInsertCritter->getWidth()) / 2,
                                    (768 - _formInsertCritter->getHeight()) / 2);
    
    _formInsertCritter->setPosition(_formInsertCritter->getX(), -_formInsertCritter->getY());
    
    createControlHeader(_formInsertCritter, "Click on the instructions to build your critter, then press Insert",
                        Vector2(10,20), Vector2(900, 40));
    
    mInsertOK = createButton(_formInsertCritter, "Insert x 500", "", Vector2(590,230), Vector2(150, 40));
    mInsertCancel = createButton(_formInsertCritter, "Cancel", "", Vector2(740,230), Vector2(120, 40));
    mInsertCritterGenome = createContainer(_formInsertCritter, true, Vector2(20,80), Vector2(720, 56));
    mInsertClear = createButton(_formInsertCritter, "Clear", "", Vector2(755,88), Vector2(80, 40));
    
    set<char> availableInstructions = InstructionSet::getAllAvailableInstructions();
    
    float x = 20;
    float y = 150;
    for (set<char>::iterator i = availableInstructions.begin(); i != availableInstructions.end(); i++)
    {
        string s;
        s = *i;
        
        Button * pInstruction = createButton(_formInsertCritter, " ", s.c_str(), Vector2(x, y), Vector2(60, 60));
        mInsertInstructionButtons.push_back(pInstruction);
        x += 60;
    }
    
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
            if (control == _speedSlider)
                Parameters::speed = _speedSlider->getValue();
            else if (control == _mutationSlider)
                Parameters::mutationPercent = _mutationSlider->getValue();
            else if (control == _cellSizeSlider)
                Parameters::cellSize = _cellSizeSlider->getValue();
            else if (control == _cycleEnergyCostSlider)
                Parameters::cycleEnergyCost = _cycleEnergyCostSlider->getValue();
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
    updateControlLabel("cycleEnergyCost", "-%.1f", Parameters::cycleEnergyCost);
    updateControlLabel("photoSynthesizeEnergyGain", "+%.1f", Parameters::photoSynthesizeEnergyGain);
    updateControlLabel("moveEnergyCost", "-%.1f", Parameters::moveEnergyCost);
    updateControlLabel("moveAndEatEnergyCost", "-%.1f", Parameters::moveAndEatEnergyCost);
    updateControlLabel("baseSpawnEnergy", "%d", (int) Parameters::baseSpawnEnergy);
    updateControlLabel("extraSpawnEnergyPerSegment", "%d", (int) Parameters::extraSpawnEnergyPerSegment);
    updateControlLabel("lookDistance", "%d", Parameters::lookDistance);
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
    
    this->_formAdvanced->update(1);
}

/**
 * Methods for programmatically creating the UI
 */
#define LABEL_WIDTH 150
#define SLIDER_WIDTH 100

Form * Main :: createForm(float width, float height, bool isLayoutVertical)
{
    Form * result = Form::create(isLayoutVertical ? "res/layoutVertical.form" : "res/layoutAbsolute.form");
    
    Container *pMainContainer = (Container*)result->getControl("main");
    pMainContainer->setSkinColor(Vector4(0,0,0,1));
    //    result->setMargin(2,2,2,2);
    
    result->setSize(width, height);
    pMainContainer->setSize(width, height - 20);
    return result;
}

void Main :: createControlHeader(Form *form, std::string text, Vector2 pos, Vector2 size)
{
    Theme::Style * pNoBorder = form->getTheme()->getStyle("noBorder");
    
    Container *pMainContainer = (Container*)form->getControl("main");
    
    if (size.x == -1)
    {
        size.x = 300;
        size.y = pMainContainer->getControls().size() ? 40 : 25;
    }
    
    Label *pLabel = Label::create("", pNoBorder);
    pLabel->setZIndex(pMainContainer->getControls().size());
    pMainContainer->addControl(pLabel);
    pLabel->setTextAlignment(Font::ALIGN_BOTTOM_LEFT);
    pLabel->setText(text.c_str());
    pLabel->setSize(size.x, size.y);
    
    if (pos.x != -1)
        pLabel->setPosition(pos.x, pos.y);
}

Container * Main :: createContainer(Form *form, bool border, Vector2 pos, Vector2 size)
{
    Theme::Style * pNoBorder = form->getTheme()->getStyle("noBorder");
    Theme::Style * pBasic = form->getTheme()->getStyle("basic");
    
    Container *pContainer = Container::create("", border ? pBasic : pNoBorder);
    Container *pMainContainer = (Container*)form->getControl("main");
    float height = pMainContainer->getControls().size() ? 40 : 25;
    pContainer->setSize(300, height);
    pContainer->setZIndex(pMainContainer->getControls().size());
    pMainContainer->addControl(pContainer);
    
    if (pos.x != -1)
        pContainer->setPosition(pos.x, pos.y);
    
    if (size.x != -1)
        pContainer->setSize(size.x, size.y);
    return pContainer;
}

void Main :: createSpacer(Form *form, float height)
{
    Theme::Style * pNoBorder = form->getTheme()->getStyle("noBorder");
    
    Container *pContainer = Container::create("", pNoBorder);
    Container *pMainContainer = (Container*)form->getControl("main");
    pContainer->setSize(300, height);
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
    pContainer->setSize(300, 30);
    
    Label *pLabel = Label::create("", pNoBorder);
    pContainer->addControl(pLabel);
    pLabel->setTextAlignment(Font::ALIGN_BOTTOM_LEFT);
    pLabel->setText(label.c_str());
    pLabel->setSize(LABEL_WIDTH, 30);
    pLabel->setPosition(5, 0);
    
    Slider *pSlider = Slider::create(id.c_str(), pNoBorder);
    pContainer->addControl(pSlider);
    pSlider->setSize(SLIDER_WIDTH, 30);
    pSlider->setPosition(LABEL_WIDTH,0);
    pSlider->setMin(minValue);
    pSlider->setMax(maxValue);
    pSlider->setStep(step);
    
    Label *pValueLabel = Label::create((id + "Label").c_str(), pNoBorder);
    pContainer->addControl(pValueLabel);
    pValueLabel->setTextAlignment(Font::ALIGN_BOTTOM_LEFT);
    pValueLabel->setSize(60, 30);
    pValueLabel->setPosition(SLIDER_WIDTH + LABEL_WIDTH, 0);
    
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
    pCheckbox->setSize(size.x, size.y);
    pCheckbox->setImageSize(20, 20);
    
    pCheckbox->addListener(this, Listener::VALUE_CHANGED);
    
    if (pos.x != -1)
        pCheckbox->setPosition(pos.x, pos.y);
    
    return pCheckbox;
}

Button * Main :: createButton(Form *form, std::string label, const char * id, Vector2 pos, Vector2 size)
{
    Theme::Style * pNoBorder = form->getTheme()->getStyle("noBorder");
    Theme::Style * pBasic = form->getTheme()->getStyle("basic");
    
    if (size.x == -1)
    {
        size.x = 280;
        size.y = 40;
    }
    
    Container *pMainContainer = (Container*)form->getControl("main");
    
    Button *pButton = Button::create(id, pBasic);
    pButton->setZIndex(pMainContainer->getControls().size());
    pMainContainer->addControl(pButton);
    pButton->setText(label.c_str());
    pButton->setSize(size.x, size.y);
    
    pButton->addListener(this, Listener::CLICK);
    
    if (pos.x != -1)
        pButton->setPosition(pos.x, pos.y);
    
    pButton->setFontSize(24);
    return pButton;
}



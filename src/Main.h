#ifndef TEMPLATEGAME_H_
#define TEMPLATEGAME_H_

#include "gameplay.h"
#include "arcball.h"
#include <pthread.h>

#ifdef _WINDOWS
extern void usleep(unsigned long ms);
#endif

using namespace gameplay;

class SphereWorld;

/**
 * Main game class.
 */
class Main: public Game, Control::Listener
{
public:

    /**
     * Constructor.
     */
    Main();

    /**
     * @see Game::keyEvent
     */
	void keyEvent(Keyboard::KeyEvent evt, int key);
	
    /**
     * @see Game::touchEvent
     */
    void touchEvent(Touch::TouchEvent evt, int x, int y, unsigned int contactIndex);
	void touchEvent(Touch::TouchEvent evt, float x, float y, unsigned int contactIndex);
    void gesturePinchEvent(int x, int y, float scale);
    
    void safeExit(void);
    
protected:

    /**
     * @see Game::initialize
     */
    void initialize();
    void createUI();
    
    /**
     * @see Game::finalize
     */
    void finalize();

    /**
     * @see Game::update
     */
    void update(float elapsedTime);

    /**
     * @see Game::render
     */
    void render(float elapsedTime);
	void beginRender(float elapsedTime);
	void renderSphere(float elapsedTime);
	void renderCritters(float elapsedTime);
	void renderSpeciesCounts(float elapsedTime);
	void renderForms(float elapsedTime);
	void finishRender(float elapsedTime);

    void renderInsertCritter();
    
    Rectangle scaleUI(Rectangle);
	Rectangle getRectangleForPoint(Vector3 point, float renderSize, float offsetX, float offsetY, float scaleSize = 1);

	/**
     * @see Control::controlEvent
     */
    void controlEvent(Control* control, EventType evt);

    void updateControlLabels();
    void setControlValues();
    void createSpacer(Form *form, float height);
    
    Label * createControlHeader(Form *form, std::string text,
                             Vector2 pos = Vector2(-1,-1),
                             Vector2 size = Vector2(-1,-1),
                             Vector4 textColor = Vector4(.5,.5,.5,1));

    Slider * createSliderControl(Form *form, std::string id, std::string label, float minValue, float maxValue, float step = 0);

    CheckBox * createCheckboxControl(Form *form, std::string label,
                                     Vector2 pos = Vector2(-1,-1),
                                     Vector2 size = Vector2(-1,-1));

    Button * createButton(Form *form, std::string label, const char * id = "",
                          Vector2 pos = Vector2(-1,-1),
                          Vector2 size = Vector2(-1,-1));

    Label * createLabel(Form *form, std::string label, const char * id = "",
                          Vector2 pos = Vector2(-1,-1),
                          Vector2 size = Vector2(-1,-1));

    Container * createContainer(Form *form, bool border,
                                Vector2 pos = Vector2(-1,-1),
                                Vector2 size = Vector2(-1,-1));
    
    Form * createForm(float width, float height, bool isLayoutVertical = true, bool framing = true);

	void createInsertCritterForm();
	void handleInsertCritterEvent(Control* control, EventType evt);
	void updateInsertCritterForm();
    void insertCritter(int count);
	void setInsertCritterFormVisible(bool);

	void createLoadSaveForm();
	void setSaveLoadFormVisible(bool show);
	void handleSaveLoad();
	bool handleSaveLoadEvent(Control* control, EventType evt);
	void updateSaveLoad(float elapsed);

    void handleGenealogy();
    
	void renderSegment(char segment, Rectangle dstRect);

    void updateControlLabel(std::string parameterId, const char *pFormat, ...);

    void setBarriers(int type, bool bOn);
    void onBarriers();
    void resetWorld();
    void resetParameters();
    
private:
	void handleSave(int i);
	void handleLoad(int i);

	void handleFollowCritter(float elapsedTime);

    static void * threadFunction(void*);
    void drawSplash(void* param);
	
	SpriteBatch * getLegalSpriteBatch(int iBatch);
    void draw(int iBatch, const Rectangle& dst, const Rectangle& src, const Vector4& color = Vector4::one());
    void draw(int iBatch, const Vector3& dst, const Rectangle& src, const Vector2& scale, const Vector4& color = Vector4::one());
    void draw(int iBatch, const Vector3& dst, const Rectangle& src, const Vector2& scale, const Vector4& color,
              const Vector2& rotationPoint, float rotationAngle);
    void draw(int iBatch, const Vector3& dst, float width, float height, float u1, float v1, float u2, float v2, const Vector4& color,
              const Vector2& rotationPoint, float rotationAngle, bool positionIsCenter = false);
    void draw(int iBatch, float x, float y, float z, float width, float height, float u1, float v1, float u2, float v2, const Vector4& color,
              const Vector2& rotationPoint, float rotationAngle, bool positionIsCenter = false);
    void draw(int iBatch, const Vector3& position, const Vector3& right, const Vector3& forward, float width, float height, 
              float u1, float v1, float u2, float v2, const Vector4& color, const Vector2& rotationPoint, float rotationAngle);
    void draw(int iBatch, float x, float y, float width, float height, float u1, float v1, float u2, float v2, const Vector4& color);
    void draw(int iBatch, float x, float y, float width, float height, float u1, float v1, float u2, float v2, const Vector4& color, const Rectangle& clip);
    void draw(int iBatch, float x, float y, float z, float width, float height, float u1, float v1, float u2, float v2, const Vector4& color, bool positionIsCenter = false);

    void openURL(const char *pPath, bool externalBrowser = false);
    
private:
	static SphereWorld world;
	int mCurBarriers;
    float mUIScale;
    ArcBall _arcball;
    pthread_t mThread;
    
    float mRenderSphereSize;
    float mSphereOffsetX;
    float mSphereOffsetY;
    
    float  mViewScale;
    Matrix mViewRotateMatrix;

    Form* _formMain;
    Form* _formAdvanced;
    Form* _formHelp;
    Form* _formClose;
    
    bool    mShowingInsertCritter;
	bool	mShowingLoadSave;
    Form* _formInsertCritter;
    Form* _formSaveLoad;
    
    Font* _font;

    Button * _saveLoadButton;
    Button * _genealogyButton;
	Button * _resetWorldButton;
    Button * _resetParametersButton;
    Button * _insertButton;
    Button * _helpButton;
    Button * _closeButton;
    
    CheckBox * _showAdvanced;
    CheckBox * _followCritter;
    CheckBox * _colorCodeSpecies;
    Slider* _speedSlider;
    Slider* _mutationSlider;
    //Slider* _cellSizeSlider;
    Slider* _randomFoodSlider;
    Slider* _barriersSlider;

    Slider* _cycleEnergyCostSlider;
    Slider* _photoSynthesizeEnergyGainSlider;
    Slider* _photoSynthesizeBonusSlider;
    Slider* _deadCellDormancySlider;
    Slider* _moveEnergyCostSlider;
    Slider* _moveAndEatEnergyCostSlider;
    //Slider* _mouthSizeSlider;

    Slider* _baseSpawnEnergySlider;
    Slider* _extraSpawnEnergyPerSegmentSlider;

    Slider* _extraCyclesForMoveSlider;
    Slider* _biteStrengthSlider;
    Slider* _digestionEfficiencySlider;

    Slider* _lookDistanceSlider;
    CheckBox * _allowSelfOverlap;
    CheckBox * _starveBecomeFood;
    CheckBox * _cannibals;
    CheckBox * _allowOr;
    CheckBox * _useNaturalMovement;

    // insert form
    std::vector<Button*> mInsertInstructionButtons;
    Container * mInsertCritterGenome;
    Button * mInsertClear;
    Button * mInsertOK_1;
    Button * mInsertOK_25;
    Button * mInsertOK_500;
    Button * mInsertCancel;
    std::string mInsertGenome;
    
    bool mShowingWebPage;
    
    SpriteBatch * mSegmentBatch[256];
    int mSegmentBatchCount[256];
	Rectangle mSegmentSrcRect[256];
};

enum
{
    iSpriteSphere = 0,
    iSpriteSphereRed,
    
    iGenericSegment,

    iSegmentFrame,

    iActiveSegment = 200,
	iActiveSegmentConditionOff,
	iSegmentIf,
	iSegmentIfNot,
	iSegmentAlways,
    iMoveArrow
};

class LockWorldMutex {
public:
	LockWorldMutex() { pthread_mutex_lock( &mMutex ); }
	~LockWorldMutex() { pthread_mutex_unlock( &mMutex ); }

private:
	static pthread_mutex_t mMutex;
};

#endif

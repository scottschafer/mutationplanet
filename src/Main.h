#ifndef TEMPLATEGAME_H_
#define TEMPLATEGAME_H_

#include "gameplay.h"
#include "arcball.h"

using namespace gameplay;

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
    void gesturePinchEvent(int x, int y, float scale);

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
    void renderInsertCritter();
    void renderHelp();
    
    /**
     * @see Control::controlEvent
     */
    void controlEvent(Control* control, EventType evt);

    void updateControlLabels();
    void setControlValues();
    void createSpacer(Form *form, float height);
    
    void createControlHeader(Form *form, std::string text,
                             Vector2 pos = Vector2(-1,-1),
                             Vector2 size = Vector2(-1,-1));

    Slider * createSliderControl(Form *form, std::string id, std::string label, float minValue, float maxValue, float step = 0);

    CheckBox * createCheckboxControl(Form *form, std::string label,
                                     Vector2 pos = Vector2(-1,-1),
                                     Vector2 size = Vector2(-1,-1));

    Button * createButton(Form *form, std::string label, const char * id = "",
                          Vector2 pos = Vector2(-1,-1),
                          Vector2 size = Vector2(-1,-1));

    Container * createContainer(Form *form, bool border,
                                Vector2 pos = Vector2(-1,-1),
                                Vector2 size = Vector2(-1,-1));
    
    Form * createForm(float width, float height, bool isLayoutVertical = true);
    
    void updateControlLabel(std::string parameterId, const char *pFormat, ...);

    void setBarriers(int type, bool bOn);
    
    void resetWorld();
    void resetParameters();
    void insertCritter();
    
private:
    static void * threadFunction(void*);
    void drawSplash(void* param);

private:
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
    
    bool    mShowingInsertCritter;
    Form* _formInsertCritter;
    
    Font* _font;

    Button * _resetWorldButton;
    Button * _resetParametersButton;
    Button * _insertButton;
    Button * _helpButton;
    
    CheckBox * _barriers1;
    CheckBox * _barriers2;
    CheckBox * _barriers3;

    CheckBox * _showAdvanced;
    Slider* _speedSlider;
    Slider* _mutationSlider;
    Slider* _cellSizeSlider;

    Slider* _cycleEnergyCostSlider;
    Slider* _photoSynthesizeEnergyGainSlider;
    Slider* _moveEnergyCostSlider;
    Slider* _moveAndEatEnergyCostSlider;

    Slider* _baseSpawnEnergySlider;
    Slider* _extraSpawnEnergyPerSegmentSlider;

    Slider* _lookDistanceSlider;
    CheckBox * _allowSelfOverlap;
    
    // insert form
    std::vector<Button*> mInsertInstructionButtons;
    Container * mInsertCritterGenome;
    Button * mInsertClear;
    Button * mInsertOK;
    Button * mInsertCancel;
    std::string mInsertGenome;
    
    // help
    bool mIsShowingHelp;
    int mHelpPageIndex;
    Rectangle mHelpPageRect;
    Rectangle mHelpCloseRect;
    
    SpriteBatch * mSegmentBatch[255];
};

#endif

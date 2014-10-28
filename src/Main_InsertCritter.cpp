#include "Main.h"
#include "SphereWorld.h"
#include "InstructionSet.h"
#include "Agent.h"
#include "UtilsRandom.h"
#include "Parameters.h"

extern Vector3 getRandomSpherePoint();

void Main :: setInsertCritterFormVisible(bool show)
{
	if (show != mShowingInsertCritter) {

		if (! show) {
			_formInsertCritter->setPosition(-_formInsertCritter->getWidth(), -_formInsertCritter->getHeight());
		}
		else {
			_formInsertCritter->setPosition((1024*mUIScale - _formInsertCritter->getWidth()) / 2,
				(768*mUIScale - _formInsertCritter->getHeight()) / 3);
		}

		mShowingInsertCritter = show;
		_formMain->setEnabled(! show);
	}
}

void Main :: updateInsertCritterForm()
{	
	bool showOK = (mInsertGenome.length() > 0);

	Button * buttons[3];
	buttons[0] = mInsertOK_1;
	buttons[1] = mInsertOK_25;
	buttons[2] = mInsertOK_500;

	for (int i = 0; i < 3; i++) {
		Button * ok = buttons[i];
		if ((ok->getY() > 0) != showOK) {
			ok->setPosition(ok->getX(), -ok->getY());
		}
	}
}

void Main :: insertCritter(int count)
{
	if (mInsertGenome.length()) {
        LockWorldMutex m;
        world.reserveAgentCount(count);
		for (int i = 0; i < count; i++)
		{
			Agent *pAgent = world.createEmptyAgent(true);
			pAgent->initialize(getRandomSpherePoint(), mInsertGenome.c_str(), true);
			world.addAgentToWorld(pAgent);
		}
	}
}

void Main :: createInsertCritterForm()
{
	_formInsertCritter = createForm(900, 400, false);
	_formInsertCritter->setPosition(-_formInsertCritter->getWidth(), -_formInsertCritter->getHeight());

	mInsertOK_1 = createButton(_formInsertCritter, "Insert", "", Vector2(120,330), Vector2(180, 40));
	mInsertOK_25 = createButton(_formInsertCritter, "Insert x 25", "", Vector2(320,330), Vector2(180, 40));
	mInsertOK_500 = createButton(_formInsertCritter, "Insert x 500", "", Vector2(520,330), Vector2(180, 40));
	mInsertCancel = createButton(_formInsertCritter, "Cancel", "", Vector2(740,330), Vector2(120, 40));
	mInsertCritterGenome = createContainer(_formInsertCritter, true, Vector2(20,20), Vector2(720, 56));
	mInsertClear = createButton(_formInsertCritter, "Clear", "", Vector2(755,28), Vector2(80, 40));
	updateInsertCritterForm();
	setInsertCritterFormVisible(false);

	set<char> availableInstructions = InstructionSet::getAllAvailableInstructions();

	float x = 20;
	float y = 80;
	for (set<char>::iterator i = availableInstructions.begin(); i != availableInstructions.end(); i++)
	{
        bool supportsConditions = InstructionSet::instructionSupportsConditions(*i);
        
        string idAlways, idIf, idNotIf;
        if (supportsConditions) {
            idAlways += (*i | eAlways);
        }
        else {
			idAlways += (*i | eAlways);
        }
        idIf += (*i | eIf);
        idNotIf += (*i | eNotIf);

        float yb = y;
		Button * pInstruction = createButton(_formInsertCritter, " ", idAlways.c_str(), Vector2(x, yb), Vector2(60, 60));
		mInsertInstructionButtons.push_back(pInstruction);

		if (supportsConditions) {
			yb += 60;

			Button * pInstructionIf = createButton(_formInsertCritter, " ", idIf.c_str(), Vector2(x, yb), Vector2(60, 60));
			mInsertInstructionButtons.push_back(pInstructionIf);
			yb += 60;

			Button * pInstructionNotIf = createButton(_formInsertCritter, " ", idNotIf.c_str(), Vector2(x, yb), Vector2(60, 60));
			mInsertInstructionButtons.push_back(pInstructionNotIf);
		}
		x += 60;
	}

	createControlHeader(_formInsertCritter, "Click on the instructions above to build your genome, then press 'Insert x 500'.\nClick on 'What is this?' for a full description of each instruction.",
		Vector2(40,250), Vector2(900, 60), Vector4(1,1,1,1));
}

void Main::renderSegment(char i, Rectangle dst)
{
    char instruction = i & eInstructionMask;
    char condition = i & eExecTypeMask;
    
	SpriteBatch * pBatch = mSegmentBatch[instruction];
	if (! pBatch)
		pBatch = mSegmentBatch[iGenericSegment];
	Rectangle src(0,0,
		pBatch->getSampler()->getTexture()->getWidth(), pBatch->getSampler()->getTexture()->getHeight());

	pBatch->draw(dst, src);

    if ((condition == eIf) || (condition == eNotIf)) {
        int iCondition = (condition == eIf) ? iSegmentIf : iSegmentIfNot;

        SpriteBatch * pBatch = mSegmentBatch[iCondition];
        Rectangle src(0,0,
            pBatch->getSampler()->getTexture()->getWidth(), pBatch->getSampler()->getTexture()->getHeight());
        pBatch->draw(dst, src);
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
			string buttonId = pButton->getId();

			Rectangle dst = pButton->getBounds();
			dst.x += _formInsertCritter->getX() + 10 * mUIScale;
			dst.y += _formInsertCritter->getY() + 8 * mUIScale;
			dst.inflate(-6 * mUIScale,-6 * mUIScale);

			renderSegment(buttonId[0], dst);
		}

		float x = mInsertCritterGenome->getX() + _formInsertCritter->getX() + 14 * mUIScale;
		float y = mInsertCritterGenome->getY() + _formInsertCritter->getY() + 10 * mUIScale;
		float w = 5 * mUIScale + mInsertCritterGenome->getWidth() / MAX_GENOME_LENGTH;
		for (string::iterator i = mInsertGenome.begin(); i != mInsertGenome.end(); i++)
		{
			Rectangle dst(x, y, w, w);
			dst.inflate(-6 * mUIScale, -6 * mUIScale);

			renderSegment(*i, dst);
			x += w - 6 * mUIScale;
		}

		for (int i = iSpriteSphere; i < 255; i++)
			if (mSegmentBatch[i])
				mSegmentBatch[i]->finish();
	}
}

void Main :: handleInsertCritterEvent(Control* control, EventType evt)
{
	if (control == _insertButton)
	{
        mInsertGenome.clear();
        updateInsertCritterForm();
		setInsertCritterFormVisible(true);

	} else if (control == mInsertClear)
	{
		if (mInsertGenome.length())
		{
			mInsertGenome = mInsertGenome.substr(0, mInsertGenome.length()-1);
			updateInsertCritterForm();
		}
	} else if (control == mInsertCancel || control == mInsertOK_1 || control == mInsertOK_25 || control == mInsertOK_500)
	{
		int nCount = 0;
		if (control == mInsertOK_1)
			nCount = 1;
		else if (control == mInsertOK_25)
			nCount = 25;
		else if (control == mInsertOK_500)
			nCount = 500;

		if (nCount > 0)
			insertCritter(nCount);

		setInsertCritterFormVisible(false);
	}
	else if (find(mInsertInstructionButtons.begin(), mInsertInstructionButtons.end(),control) !=
		mInsertInstructionButtons.end())
	{
		if (mInsertGenome.length() < MAX_GENOME_LENGTH)
			mInsertGenome += control->getId();

		updateInsertCritterForm();
	}
}
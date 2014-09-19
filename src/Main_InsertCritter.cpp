#include "Main.h"
#include "SphereWorld.h"
#include "InstructionSet.h"
#include "Agent.h"
#include "UtilsRandom.h"
#include "Parameters.h"

extern pthread_mutex_t mutex1;
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
		pthread_mutex_lock( &mutex1 );

		vector<Instruction> instructions;
		for (std::string::iterator i = mInsertGenome.begin(); i != mInsertGenome.end(); i++)
		{
			Instruction instruction;
			instruction.instruction = *i;
			++i;

			if (*i == eIf)
				instruction.executeType = eIf;
			else
				if (*i == eNotIf)
					instruction.executeType = eNotIf;
				else
					instruction.executeType = eAlways;

			instructions.push_back(instruction);
		}

		Instruction iNull;
		iNull.instruction = 0;
		instructions.push_back(iNull);

		for (int i = 0; i < count; i++)
		{
			Agent *pAgent = world.createEmptyAgent(true);
			pAgent->initialize(getRandomSpherePoint(), instructions.data(), true);
			world.addAgentToWorld(pAgent);
		}
		pthread_mutex_unlock( &mutex1 );
	}
}

void Main :: createInsertCritterForm()
{
	_formInsertCritter = createForm(900, 430, false);
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
		string s;
		s = *i;

		string sAlways = s;
		sAlways += eAlways;
		float yb = y;
		Button * pInstruction = createButton(_formInsertCritter, " ", sAlways.c_str(), Vector2(x, yb), Vector2(60, 60));
		mInsertInstructionButtons.push_back(pInstruction);

		if (InstructionSet::instructionSupportsConditions(s[0])) {
			yb += 60;

			string sIf = s;
			sIf += eIf;
			string sNotIf = s;
			sNotIf += eNotIf;

			Button * pInstructionIf = createButton(_formInsertCritter, " ", sIf.c_str(), Vector2(x, yb), Vector2(60, 60));
			mInsertInstructionButtons.push_back(pInstructionIf);
			yb += 60;

			Button * pInstructionNotIf = createButton(_formInsertCritter, " ", sNotIf.c_str(), Vector2(x, yb), Vector2(60, 60));
			mInsertInstructionButtons.push_back(pInstructionNotIf);
		}
		x += 60;
	}

	createControlHeader(_formInsertCritter, "Click on the instructions above to build your genome, then press 'Insert x 500'.\nClick on 'What is this?' for a full description of each instruction.",
		Vector2(40,250), Vector2(900, 60), Vector4(1,1,1,1));
}

void Main::renderSegment(string segment, Rectangle dst)
{
	SpriteBatch * pBatch = mSegmentBatch[segment[0]];
	if (! pBatch)
		pBatch = mSegmentBatch[iGenericSegment];
	Rectangle src(0,0,
		pBatch->getSampler()->getTexture()->getWidth(), pBatch->getSampler()->getTexture()->getHeight());

	pBatch->draw(dst, src);

	if (segment.size() == 2)
	{
		char condition = segment[1];
		if ((condition == eIf) || (condition == eNotIf)) {
			int iCondition = (condition == eIf) ? iSegmentIf : iSegmentIfNot;

			SpriteBatch * pBatch = mSegmentBatch[iCondition];
			Rectangle src(0,0,
				pBatch->getSampler()->getTexture()->getWidth(), pBatch->getSampler()->getTexture()->getHeight());
			pBatch->draw(dst, src);
		}
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
			dst.x += _formInsertCritter->getX() + 20 * mUIScale;
			dst.y += _formInsertCritter->getY() + 18 * mUIScale;
			dst.inflate(-6 * mUIScale,-6 * mUIScale);

			renderSegment(buttonId, dst);
		}

		float x = mInsertCritterGenome->getX() + _formInsertCritter->getX() + 24 * mUIScale;
		float y = mInsertCritterGenome->getY() + _formInsertCritter->getY() + 20 * mUIScale;
		float w = 5 * mUIScale + mInsertCritterGenome->getWidth() / MAX_GENOME_LENGTH;
		for (string::iterator i = mInsertGenome.begin(); i != mInsertGenome.end(); i++)
		{
			string segment;
			segment += *i;
			++i;
			segment += *i;

			Rectangle dst(x, y, w, w);
			dst.inflate(-6 * mUIScale, -6 * mUIScale);

			renderSegment(segment, dst);
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
		mInsertGenome = "";
		updateInsertCritterForm();
		setInsertCritterFormVisible(true);

	} else if (control == mInsertClear)
	{
		if (mInsertGenome.length())
		{
			mInsertGenome = mInsertGenome.substr(0, mInsertGenome.length()-2);
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
		if (mInsertGenome.length() < MAX_GENOME_LENGTH*2)
			mInsertGenome += control->getId();

		updateInsertCritterForm();
	}
}
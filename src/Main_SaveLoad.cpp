#include "Main.h"
#include "SphereWorld.h"
#include "InstructionSet.h"
#include "Agent.h"
#include "UtilsRandom.h"
#include "Parameters.h"
#include <fstream>

extern pthread_mutex_t mutex1;

bool saved = false;

static Button * mCancel;
static Button * mSave[5];
static Button * mLoad[5];
static Button * mResetWorld;
static Label * mStatusLabel;

static Button * mClickedButton = NULL;
static float executeButtonTime = 0;

void Main :: createLoadSaveForm()
{
	_formSaveLoad = createForm(900, 280, false);
	_formSaveLoad->setPosition(-_formSaveLoad->getWidth(), -_formSaveLoad->getHeight());

	createControlHeader(_formSaveLoad, "Save and load your world using the available slots.",
		Vector2(20,20), Vector2(900, 20), Vector4(1,1,1,1));

	mStatusLabel = createControlHeader(_formSaveLoad, "",
		Vector2(20,190), Vector2(900, 40), Vector4(1,1,1,1));

	char buffer[200];
	for (int i = 0; i < sizeof(mSave)/sizeof(mSave[0]); i++) {
		sprintf(buffer, "Save #%d", i  +1);
		mSave[i] = createButton(_formSaveLoad, buffer, "", Vector2(40+150*i,70), Vector2(140, 40));

		sprintf(buffer, "Load #%d", i  +1);
		mLoad[i] = createButton(_formSaveLoad, buffer, "", Vector2(40+150*i,120), Vector2(140, 40));
	}

	mResetWorld = createButton(_formSaveLoad, "Reset World", "", Vector2(580,190), Vector2(150, 40));
	mCancel = createButton(_formSaveLoad, "Cancel", "", Vector2(740,190), Vector2(120, 40));
	setSaveLoadFormVisible(false);
}

bool Main :: handleSaveLoadEvent(Control* control, EventType evt)
{
	executeButtonTime = 0;
	if (control == mCancel) {
		setSaveLoadFormVisible(false);
		return true;
	}
	else if (control == mResetWorld) {
		setSaveLoadFormVisible(false);
		resetWorld();
		return true;
	}
	else {
		for (int i = 0; i < sizeof(mSave)/sizeof(mSave[0]); i++) {
			if (control == mSave[i]) {
				mStatusLabel->setText("Saving...");
				mClickedButton = (Button*)control;
				return true;
			}
			if (control == mLoad[i]) {
				mStatusLabel->setText("Loading...");
				mClickedButton = (Button*)control;
				return true;
			}
		}
	}
	return false;
}

void Main :: updateSaveLoad(float elapsed)
{
	if (mClickedButton != NULL)
	{
		if (executeButtonTime == 0)
		{
			executeButtonTime = elapsed + 1;
			return;
		}
		if (elapsed < executeButtonTime) {
			return;
		}

		Button * clickedButton = mClickedButton;
		mClickedButton = NULL;

		for (int i = 0; i < sizeof(mSave)/sizeof(mSave[0]); i++) {
			if (clickedButton == mSave[i]) {
				handleSave(i);
				break;
			}
			if (clickedButton == mLoad[i]) {
				handleLoad(i);
				break;
			}
		}
		setSaveLoadFormVisible(false);
	}
}

void Main :: setSaveLoadFormVisible(bool show)
{
	if (show != mShowingLoadSave) {

		if (show) {
			mStatusLabel->setText("");
			mClickedButton = NULL;

			for (int i = 0; i < sizeof(mSave)/sizeof(mSave[0]); i++) {
				char fileName[200];
				sprintf(fileName, "World %d", i);
				FILE * pTest = fopen(fileName, "r");
				mLoad[i]->setVisible(pTest != NULL);
				if (pTest != NULL) {
					fclose(pTest);
				}
			}
		}

		if (! show) {
			_formSaveLoad->setPosition(-_formSaveLoad->getWidth(), -_formSaveLoad->getHeight());
		}
		else {
			_formSaveLoad->setPosition((1024*mUIScale - _formSaveLoad->getWidth()) / 2,
				(768*mUIScale - _formSaveLoad->getHeight()) / 3);
		}

		mShowingLoadSave = show;
		_formMain->setEnabled(! show);
	}
}

void Main :: handleSaveLoad()
{
	setSaveLoadFormVisible(true);
}

void Main :: handleSave(int i)
{
    pthread_mutex_lock( &mutex1 );
	ofstream out;
	char fileName[200];
	sprintf(fileName, "World %d", i);
	out.open(fileName, ios::binary);

	static int endianIndicator = 1;
	static int version = 1;

	out.write((char*)&endianIndicator, sizeof(endianIndicator));
	out.write((char*)&version, sizeof(version));

	out.write((char*)&Parameters::instance, sizeof(Parameters));
	world.write(out);
	pthread_mutex_unlock( &mutex1 );
}

void Main :: handleLoad(int i)
{
    pthread_mutex_lock( &mutex1 );
	world.clear();

	ifstream in;
	char fileName[200];
	sprintf(fileName, "World %d", i);

	in.open(fileName, ios::in | ios::binary);

	int endianIndicator = 1;
	int version = 1;

	in.read((char*)&endianIndicator, sizeof(endianIndicator));
	in.read((char*)&version, sizeof(version));

	in.read((char*)&Parameters::instance, sizeof(Parameters));
	world.read(in);
	in.close();

	setControlValues();
	updateControlLabels();
	pthread_mutex_unlock( &mutex1 );
}
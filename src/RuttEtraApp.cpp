#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/ip/EdgeDetect.h"
#include "cinder/ip/Blend.h"

// Settings
#include "SDASettings.h"
// Session
#include "SDASession.h"
// Log
#include "SDALog.h"
// Spout
#include "CiSpoutOut.h"

using namespace ci;
using namespace ci::app;
using namespace std;
using namespace SophiaDigitalArt;

class RuttEtraApp : public App {

public:
	RuttEtraApp();
	void mouseMove(MouseEvent event) override;
	void mouseDown(MouseEvent event) override;
	void mouseDrag(MouseEvent event) override;
	void mouseUp(MouseEvent event) override;
	void keyDown(KeyEvent event) override;
	void keyUp(KeyEvent event) override;
	void fileDrop(FileDropEvent event) override;
	void update() override;
	void draw() override;
	void cleanup() override;
	void setUIVisibility(bool visible);
private:
	// Settings
	SDASettingsRef					mSDASettings;
	// Session
	SDASessionRef					mSDASession;
	// Log
	SDALogRef						mSDALog;
	// imgui
	float							color[4];
	float							backcolor[4];
	int								playheadPositions[12];
	int								speeds[12];

	float							f = 0.0f;
	char							buf[64];
	unsigned int					i, j;

	bool							mouseGlobal;

	string							mError;
	// fbo
	bool							mIsShutDown;
	Anim<float>						mRenderWindowTimer;
	void							positionRenderWindow();
	bool							mFadeInDelay;
	SpoutOut 						mSpoutOut;
	// ruttetra
	CaptureRef mCapture;
	gl::Texture2dRef mTexture;
	vector <vec3> pixData;

	int stepX, stepY, lineBreak;
	int blackBreak;
	float elevation;
	bool enableSobel, enableWebcamPreview, enableGrayScale;
	Surface8uRef edge;
	string pos;

};


RuttEtraApp::RuttEtraApp() : mSpoutOut("SDA RuttEtra", app::getWindowSize())
{
	// Settings
	mSDASettings = SDASettings::create();
	// Session
	mSDASession = SDASession::create(mSDASettings);
	//mSDASettings->mCursorVisible = true;
	setUIVisibility(mSDASettings->mCursorVisible);
	mSDASession->getWindowsResolution();

	mouseGlobal = false;
	mFadeInDelay = true;
	// windows
	mIsShutDown = false;
	mRenderWindowTimer = 0.0f;
	timeline().apply(&mRenderWindowTimer, 1.0f, 2.0f).finishFn([&] { positionRenderWindow(); });
	// RuttEtra
	elevation = 10;
	enableWebcamPreview = enableSobel = false;
	enableGrayScale = true;

	try {
		mCapture = Capture::create(640, 480);
		mCapture->start();
	}
	catch (...) {
		console() << "Failed to initialize capture" << std::endl;
	}

	for (int i = 0; i < mCapture->getWidth()*mCapture->getHeight(); i++)
	{
		pixData.push_back(vec3(0.0f));
	}
	stepY = 5;
	stepX = 5;
	blackBreak = 64;
	edge = Surface8u::create(mCapture->getWidth(), mCapture->getHeight(), false, SurfaceChannelOrder::RGB);

}
void RuttEtraApp::positionRenderWindow() {
	mSDASettings->mRenderPosXY = ivec2(mSDASettings->mRenderX, mSDASettings->mRenderY);//20141214 was 0
	setWindowPos(mSDASettings->mRenderX, mSDASettings->mRenderY);
	setWindowSize(mSDASettings->mRenderWidth, mSDASettings->mRenderHeight);
}
void RuttEtraApp::setUIVisibility(bool visible)
{
	if (visible)
	{
		showCursor();
	}
	else
	{
		hideCursor();
	}
}
void RuttEtraApp::fileDrop(FileDropEvent event)
{
	mSDASession->fileDrop(event);
}
void RuttEtraApp::update()
{
	mSDASession->setFloatUniformValueByIndex(mSDASettings->IFPS, getAverageFps());
	//mSDASession->update();
	if (mCapture->isCapturing())
	{
		if (enableSobel)
		{
			//ci::ip::edgeDetectSobel(mCapture->getSurface(), &edge);
			//if (enableWebcamPreview) mTexture = gl::Texture::create(edge);

		}
		else
		{
			if (enableWebcamPreview) {
				if (!mTexture) {
					// Capture images come back as top-down, and it's more efficient to keep them that way
					mTexture = gl::Texture::create(*mCapture->getSurface(), gl::Texture::Format().loadTopDown());
				}
				else {
					mTexture->update(*mCapture->getSurface());
				}
			}
		}

		//Surface::Iter surfIter = enableSobel ? surfIter = edge.getIter(edge.getBounds()) : surfIter = mCapture->getSurface().getIter(mCapture->getBounds());
		Surface::Iter surfIter = edge->getIter(edge->getBounds());
		int p = 0;
		while (surfIter.line())
		{
			while (surfIter.pixel())
			{
				float brightness = surfIter.r() + surfIter.g() + surfIter.b();

				if (surfIter.x() % stepX == 0 && surfIter.y() % stepY == 0)
				{
					pixData[p].x = surfIter.x();
					pixData[p].y = surfIter.mWidth - surfIter.y();
					pixData[p].z = brightness / elevation; // !!!inversion
					p++;
				}

			}
		}

		lineBreak = p;
	}
	getWindow()->setTitle("(" + toString(floor(getAverageFps())) + " fps) RuttEtra");

}
void RuttEtraApp::cleanup()
{
	if (!mIsShutDown)
	{
		mIsShutDown = true;
		CI_LOG_V("shutdown");
		// save settings
		mSDASettings->save();
		mSDASession->save();
		quit();
	}
}
void RuttEtraApp::mouseMove(MouseEvent event)
{
	if (!mSDASession->handleMouseMove(event)) {
		// let your application perform its mouseMove handling here
	}
}
void RuttEtraApp::mouseDown(MouseEvent event)
{
	if (!mSDASession->handleMouseDown(event)) {
		// let your application perform its mouseDown handling here
		if (event.isRightDown()) { 
		}
	}
}
void RuttEtraApp::mouseDrag(MouseEvent event)
{
	if (!mSDASession->handleMouseDrag(event)) {
		// let your application perform its mouseDrag handling here
	}	
}
void RuttEtraApp::mouseUp(MouseEvent event)
{
	if (!mSDASession->handleMouseUp(event)) {
		// let your application perform its mouseUp handling here
	}
}

void RuttEtraApp::keyDown(KeyEvent event)
{
	if (!mSDASession->handleKeyDown(event)) {
		switch (event.getCode()) {
		case KeyEvent::KEY_ESCAPE:
			// quit the application
			quit();
			break;
		case KeyEvent::KEY_h:
			// mouse cursor and ui visibility
			mSDASettings->mCursorVisible = !mSDASettings->mCursorVisible;
			setUIVisibility(mSDASettings->mCursorVisible);
			break;
		}
	}
}
void RuttEtraApp::keyUp(KeyEvent event)
{
	if (!mSDASession->handleKeyUp(event)) {
	}
}

void RuttEtraApp::draw()
{
	gl::clear(Color(0.39f, 0.025f, 0.0f));
	/*if (mFadeInDelay) {
		mSDASettings->iAlpha = 0.0f;
		if (getElapsedFrames() > mSDASession->getFadeInDelay()) {
			mFadeInDelay = false;
			timeline().apply(&mSDASettings->iAlpha, 0.0f, 1.0f, 1.5f, EaseInCubic());
		}
	}*/
	gl::color(Color::white());
	if (enableWebcamPreview) gl::draw(mTexture, mTexture->getBounds(), getWindowBounds());
	gl::enableDepthWrite();
	gl::enableDepthRead(true);
	int drawnVertices = 0;
	gl::begin(GL_LINE_STRIP);
	int lb = 0;
	for (int x = 0; x < lineBreak; x++)
	{
		if (x < pixData.size())
		{
			if (x > 0)
			{

				if (pixData[x].z < blackBreak / (elevation / 3) || (pixData[x].x < pixData[x - 1].x && x != 0))
				{
					gl::end();
					gl::begin(GL_LINE_STRIP);

				}
				else {
					gl::vertex(pixData[x]);
					gl::color(enableGrayScale ? Color::gray(pixData[x].z / (255 * 3) * elevation) : Color::white());
					drawnVertices++;
				}
			}
		}
	}

	gl::end();

	
	gl::enableAlphaBlending();
	gl::drawString("pos : " + pos + "Points : " + toString(lineBreak) + "\n Drawn Vertices:" + toString(drawnVertices), vec2(getWindowWidth() - 120, 20), Color::black());
	gl::disableAlphaBlending(); 
	//gl::setMatricesWindow(toPixels(getWindowSize()),false);
	//gl::setMatricesWindow(mSDASettings->mRenderWidth, mSDASettings->mRenderHeight, false);
	//gl::draw(mSDASession->getMixTexture(), getWindowBounds());

	// Spout Send
	mSpoutOut.sendViewport();
	getWindow()->setTitle(mSDASettings->sFps + " fps SDA");
}

void prepareSettings(App::Settings *settings)
{
	settings->setWindowSize(640, 480);
}

CINDER_APP(RuttEtraApp, RendererGl, prepareSettings)

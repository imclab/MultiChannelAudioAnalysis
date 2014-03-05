#include "cinder/Cinder.h"

#if defined( CINDER_COCOA_TOUCH )
#include "cinder/app/AppCocoaTouch.h"
typedef ci::app::AppCocoaTouch AppBase;
#else
#include "cinder/app/AppBasic.h"
#include "cinder/audio/FftProcessor.h"
typedef ci::app::AppBasic AppBase;
#endif

#include "cinder/audio/Input.h"
#include <iostream>
#include <vector>

using namespace ci;
using namespace ci::app;

class MCAA_App : public AppBase {
public:
	void setup();
	void update();
	void draw();
	void drawWaveForm( float height );
    
	void drawFft( std::shared_ptr<float> mFftDataRef );
    
	
	audio::Input mInput;
	std::shared_ptr<float> mFftDataRefL;
    std::shared_ptr<float> mFftDataRefR;
    std::shared_ptr<float> mFftDataRefT;
	audio::PcmBuffer32fRef mPcmBuffer;
};

void MCAA_App::setup()
{
	//iterate input devices and print their names to the console
	const std::vector<audio::InputDeviceRef>& devices = audio::Input::getDevices();
	for( std::vector<audio::InputDeviceRef>::const_iterator iter = devices.begin(); iter != devices.end(); ++iter ) {
		console() << (*iter)->getName() << std::endl;
	}
    
	//initialize the audio Input, using the default input device
	mInput = audio::Input();
	
	//tell the input to start capturing audio
	mInput.start();
	
}

void MCAA_App::update()
{
	mPcmBuffer = mInput.getPcmBuffer();
	if( ! mPcmBuffer ) {
		return;
	}
    
	uint16_t bandCount = 512;
	//presently FFT only works on OS X, not iOS
	mFftDataRefL = audio::calculateFft( mPcmBuffer->getChannelData( audio::CHANNEL_FRONT_LEFT ), bandCount );
    mFftDataRefR = audio::calculateFft( mPcmBuffer->getChannelData( audio::CHANNEL_FRONT_RIGHT), bandCount );
    mFftDataRefT = audio::calculateFft( mPcmBuffer->getChannelData( audio::CHANNEL_FRONT_TOP), bandCount );
    
}

void MCAA_App::draw()
{
    
	float waveFormHeight = 100.0;
    
    
	gl::setMatricesWindow( getWindowWidth(), getWindowHeight() );
	gl::clear( Color( 0.1f, 0.1f, 0.1f ) );
	
	glPushMatrix();
    glTranslatef(0.0f, getWindowWidth()*0.2f, 0.0f);
    
    
    drawWaveForm( waveFormHeight );
    
    
    drawFft(mFftDataRefL);
    drawFft(mFftDataRefR);
    drawFft(mFftDataRefT);
    
	glPopMatrix();
}

void MCAA_App::drawWaveForm( float height )
{
	if( ! mPcmBuffer ) {
		return;
	}
	
	uint32_t bufferSamples = mPcmBuffer->getSampleCount();
    
	audio::Buffer32fRef leftBuffer = mPcmBuffer->getChannelData( audio::CHANNEL_FRONT_LEFT);
	audio::Buffer32fRef rightBuffer = mPcmBuffer->getChannelData( audio::CHANNEL_FRONT_RIGHT );
    audio::Buffer32fRef topBuffer = mPcmBuffer->getChannelData( audio::CHANNEL_FRONT_TOP );
    
	int displaySize = getWindowWidth();
	int endIdx = bufferSamples;
	
	//only draw the last 1024 samples or less
	int32_t startIdx = ( endIdx - 2048 );
	startIdx = math<int32_t>::clamp( startIdx, 0, endIdx );
	
	float scale = displaySize / (float)( endIdx - startIdx );
	
	PolyLine<Vec2f>	lineL;
    PolyLine<Vec2f>	lineR;
    PolyLine<Vec2f>	lineT;
    
	gl::color( Color( 0.0f, 0.8f, 1.0f ) );
    
	for( uint32_t i = startIdx, c = 0; i < endIdx; i++, c++ ) {
        
		float yL = ( ( leftBuffer->mData[i] - 1 ) * - 100 );
        float yR = ( ( rightBuffer->mData[i] - 1 ) *  - 100 );
        float yT = ( ( topBuffer->mData[i] - 1 ) *  - 100 );
        
        
		lineL.push_back( Vec2f( ( c * scale ), yL ) );
        lineR.push_back( Vec2f( ( c * scale ), yR ) );
               lineT.push_back( Vec2f( ( c * scale ), yT ) );
	}
    
      gl::draw( lineT );
    
    glPushMatrix();
    glTranslatef(0.0f, -100.0f, 0.0f);
    gl::draw( lineL );
    glTranslatef(0.0f, 200.0f, 0.0f);
    gl::draw( lineR );
    glPopMatrix();
	
    
}

void MCAA_App::drawFft( std::shared_ptr<float> mFftDataRef )
{
	uint16_t bandCount = 512;
	float ht = 1000.0f;
	float bottom = 150.0f;
	
	if( ! mFftDataRefL ) {
		return;
	}
	
	float * fftBuffer = mFftDataRef.get();
	
	for( int i = 0; i < ( bandCount ); i++ ) {
		float barY = fftBuffer[i] / bandCount * ht;
		glBegin( GL_QUADS );
        glColor3f( 255.0f, 255.0f, 0.0f );
        glVertex2f( i * 3, bottom );
        glVertex2f( i * 3 + 1, bottom );
        glColor3f( 0.0f, 255.0f, 0.0f );
        glVertex2f( i * 3 + 1, bottom - barY );
        glVertex2f( i * 3, bottom - barY );
		glEnd();
	}
}

CINDER_APP_BASIC( MCAA_App, RendererGl )


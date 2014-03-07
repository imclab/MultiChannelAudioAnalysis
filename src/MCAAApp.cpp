#include "cinder/Cinder.h"


#include "cinder/app/AppBasic.h"
#include "cinder/audio/FftProcessor.h"
typedef ci::app::AppBasic AppBase;

#include "cinder/audio/Input.h"
#include <iostream>
#include <vector>

using namespace ci;
using namespace ci::app;

class MCAA_App : public AppBase {
public:
    void prepareSettings( Settings *settings);
	void setup();
	void update();
	void draw();
	void drawWaveForms( float height );
    
	void drawFft( std::shared_ptr<float> mFftDataRef );
    void keyDown(KeyEvent e);
    
	
	audio::Input mInput;
	std::shared_ptr<float> mFftDataRefL;
    std::shared_ptr<float> mFftDataRefR;
    std::shared_ptr<float> mFftDataRefT;
	audio::PcmBuffer32fRef mPcmBuffer;
    
    Boolean live;
    
};

void MCAA_App::prepareSettings( Settings *settings ){
    settings->setWindowSize( 1280, 720 );
    //settings->setFrameRate( 60.0f );
}

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
    
    // sets the live to true to start
    live = true;
	
}

void MCAA_App::update()
{
    
    if (live){
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
}

void MCAA_App::draw()
{
    
    float waveFormHeight = 100.0;
    
    
    gl::setMatricesWindow( getWindowWidth(), getWindowHeight() );
    gl::clear( Color( 0.1f, 0.1f, 0.1f ) );
    
    glPushMatrix();
    glTranslatef(0.0f, getWindowHeight()*0.5f, 0.0f);
    
    
    drawWaveForms( waveFormHeight );
    
    drawFft(mFftDataRefR);
    
    glPushMatrix();
    glTranslatef(0.0f, -200.0f, 0.0f);
    drawFft(mFftDataRefL);
    glTranslatef(0.0f, 400.0f, 0.0f);
    drawFft(mFftDataRefT);
    glPopMatrix();
    
    glPopMatrix();
}

void MCAA_App::drawWaveForms( float height )
{
    if( ! mPcmBuffer ) {
        return;
    }
    
    // buffer samples is 2048 for the focusrite composite device
    
    uint32_t bufferSamples = mPcmBuffer->getSampleCount();
    //console() << bufferSamples << std::endl;
    
    audio::Buffer32fRef leftBuffer = mPcmBuffer->getChannelData( audio::CHANNEL_FRONT_LEFT);
    audio::Buffer32fRef rightBuffer = mPcmBuffer->getChannelData( audio::CHANNEL_FRONT_RIGHT );
    audio::Buffer32fRef topBuffer = mPcmBuffer->getChannelData( audio::CHANNEL_FRONT_TOP );
    
    int displaySize = getWindowWidth();
    int endIdx = bufferSamples;
    
    //only draw the last 2048(changeable) samples or less
    int32_t startIdx = ( endIdx - 2048 );
    startIdx = math<int32_t>::clamp( startIdx, 0, endIdx );
    
    float scale = displaySize / (float)( endIdx - startIdx );
    
    PolyLine<Vec2f>	lineL;
    PolyLine<Vec2f>	lineR;
    PolyLine<Vec2f>	lineT;
    
    gl::color( Color( 0.0f, 0.8f, 1.0f ) );
    
    float maxL = 0;
    float maxR = 0;
    float maxT = 0;
    
    for( uint32_t i = startIdx, c = 0; i < endIdx; i++, c++ ) {
        
        // it seems that zero signal returns a -1 in the mdata so an extra displacement is needed
        
        float yL = ( ( leftBuffer->mData[i] - 1 ) * - 100 - 300);
        float yR = ( ( rightBuffer->mData[i] - 1 ) *  - 100 - 100);
        float yT = ( ( topBuffer->mData[i] - 1 ) *  - 100 + 100);
        
        if (abs(yL) > abs(maxL)) maxL = yL;
        if (abs(yR) > abs(maxR)) maxR = yR;
        if (abs(yT) > abs(maxT)) maxT = yT;
        
        
        lineL.push_back( Vec2f( ( c * scale ), yL ) );
        lineR.push_back( Vec2f( ( c * scale ), yR ) );
        lineT.push_back( Vec2f( ( c * scale ), yT ) );
        
        
    }
    
    if (abs(maxR) > 20 ) live = false;
    
    gl::color( Color( 1.0f, 0.0f, 0.0f ) );
    
    Vec2f st = *new Vec2f(0.0f, maxR);
    Vec2f end = *new Vec2f(getWindowWidth(), maxR);
    
    gl::drawLine(st, end);
    
    gl::color( Color( 0.0f, 0.8f, 1.0f ) );
    gl::draw( lineL );
    
    gl::draw( lineR );
    
    gl::draw( lineT );
    
    
}

void MCAA_App::drawFft( std::shared_ptr<float> mFftDataRef )
{
    if( ! mPcmBuffer ) {
        return;
    }
    
    uint16_t bandCount = 512;
    float ht = 1000.0f;
    float bottom = 0.0f;
    
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

void MCAA_App::keyDown(KeyEvent e){
//    if (e.getChar() == 's') {
//        mInput.start();
//    }
//    if (e.getChar() == 'e') {
//        mInput.stop();
//    }
    
    if (e.getChar() == 's') {
        live = true;
    }
}

CINDER_APP_BASIC( MCAA_App, RendererGl )


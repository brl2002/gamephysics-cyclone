/*
 * The BigBallistic demo.
 *
 * Part of the Cyclone physics system.
 *
 * Copyright (c) Icosagon 2003. All Rights Reserved.
 *
 * This software is distributed under licence. Use of this software
 * implies agreement with all terms and conditions of the accompanying
 * software licence.
 */

#include <gl/glut.h>
#include <cyclone/cyclone.h>
#include "../app.h"
#include "../timing.h"

#include <stdio.h>

#define DEG_TO_RAD 3.1415926 / 180.0;
#define RAD_TO_DEG 180.0 / 3.1415926;

enum ShotType
{
    UNUSED = 0,
    PISTOL,
    ARTILLERY,
    FIREBALL,
    LASER
};

enum CameraPosState
{
	LEFT,
	RIGHT,
	IN_TRANSITION
};

class AmmoRound : public cyclone::CollisionSphere
{
public:
    ShotType type;
    unsigned startTime;

    AmmoRound()
    {
        body = new cyclone::RigidBody;
    }

    ~AmmoRound()
    {
        delete body;
    }

    /** Draws the box, excluding its shadow. */
    void render()
    {
        // Get the OpenGL transformation
        GLfloat mat[16];
        body->getGLTransform(mat);

        glPushMatrix();
        glMultMatrixf(mat);
        glutSolidSphere(radius, 20, 20);
        glPopMatrix();
    }

    /** Sets the box to a specific location. */
	void setState(ShotType shotType, float orientation, CameraPosState state)
    {
        type = shotType;

		cyclone::Vector3 dir;
		cyclone::Quaternion quat;
		cyclone::Matrix4 mat = cyclone::Matrix4();
		cyclone::real orientationRad = orientation * DEG_TO_RAD;

		switch (state)
		{
		case LEFT:
			dir = cyclone::Vector3(0, 0, -1);
			quat = cyclone::Quaternion(orientationRad, -1, 0, 0);
			mat.setOrientationAndPos(quat, cyclone::Vector3(0, 0, 0));
			break;
		case RIGHT:
			dir = cyclone::Vector3(0, 0, 1);
			quat = cyclone::Quaternion(orientationRad, 1, 0, 0);
			mat.setOrientationAndPos(quat, cyclone::Vector3(0, 0, 0));
			break;
		}
		dir = mat.transformDirection(dir);

        // Set the properties of the particle
        switch(type)
        {
        case PISTOL:
			dir = dir * 25.0f;
            body->setMass(1.5f);
            body->setVelocity(dir.x, dir.y, dir.z);
            body->setAcceleration(0.0f, -9.8f, 0.0f);
            body->setDamping(0.99f, 0.8f);
            radius = 0.2f;
            break;

        case ARTILLERY:
			dir = dir * 50.0f;
            body->setMass(200.0f); // 200.0kg
            body->setVelocity(dir.x, dir.y, dir.z); // 50m/s
            body->setAcceleration(0.0f, -9.8f, 0.0f);
            body->setDamping(0.99f, 0.8f);
            radius = 0.4f;
            break;

        case FIREBALL:
            body->setMass(4.0f); // 4.0kg - mostly blast damage
            body->setVelocity(0.0f, -0.5f, 10.0); // 10m/s
            body->setAcceleration(0.0f, 0.3f, 0.0f); // Floats up
            body->setDamping(0.9f, 0.8f);
            radius = 0.6f;
            break;

        case LASER:
            // Note that this is the kind of laser bolt seen in films,
            // not a realistic laser beam!
            body->setMass(0.1f); // 0.1kg - almost no weight
            body->setVelocity(0.0f, 0.0f, 100.0f); // 100m/s
            body->setAcceleration(0.0f, 0.0f, 0.0f); // No gravity
            body->setDamping(0.99f, 0.8f);
            radius = 0.2f;
            break;
        }

        body->setCanSleep(false);
        body->setAwake();

        cyclone::Matrix3 tensor;
        cyclone::real coeff = 0.4f*body->getMass()*radius*radius;
        tensor.setInertiaTensorCoeffs(coeff,coeff,coeff);
        body->setInertiaTensor(tensor);

        // Set the data common to all particle types
		switch (state)
		{
		case LEFT:
			body->setPosition(0.0f, 1.5f, 0.0f);
			break;
		case RIGHT:
			body->setPosition(0.0f, 1.5f, 99.0f);
			break;
		}
        startTime = TimingData::get().lastFrameTimestamp;

        // Clear the force accumulators
        body->calculateDerivedData();
        calculateInternals();
    }
};

class Box : public cyclone::CollisionBox
{
public:
	cyclone::Vector3 color;

    Box()
    {
        body = new cyclone::RigidBody;
    }

    ~Box()
    {
        delete body;
    }

    /** Draws the box, excluding its shadow. */
    void render()
    {
        // Get the OpenGL transformation
        GLfloat mat[16];
        body->getGLTransform(mat);

        glPushMatrix();
        glMultMatrixf(mat);
        glScalef(halfSize.x*2, halfSize.y*2, halfSize.z*2);
        glutSolidCube(1.0f);
        glPopMatrix();
    }

    /** Sets the box to a specific location. */
    void setState(cyclone::real z)
    {
		color = cyclone::Vector3(1, 0, 0);

        body->setPosition(0, 0.5, z);
        body->setOrientation(1,0,0,0);
        body->setVelocity(0,0,0);
        body->setRotation(cyclone::Vector3(0,0,0));
        halfSize = cyclone::Vector3(1,1,1);

        cyclone::real mass = halfSize.x * halfSize.y * halfSize.z * 8.0f;
        body->setMass(mass);

        cyclone::Matrix3 tensor;
        tensor.setBlockInertiaTensor(halfSize, mass);
        body->setInertiaTensor(tensor);

        body->setLinearDamping(0.0f);
        body->setAngularDamping(0.0f);
        body->clearAccumulators();
        body->setAcceleration(0,-10.0f,0);

        body->setCanSleep(false);
        body->setAwake();

        body->calculateDerivedData();
        calculateInternals();
    }
};


/**
 * The main demo class definition.
 */
class BigBallisticDemo : public RigidBodyApplication
{
    /**
     * Holds the maximum number of  rounds that can be
     * fired.
     */
    const static unsigned ammoRounds = 256;

    /** Holds the particle data. */
    AmmoRound ammo[ammoRounds];

    /**
    * Holds the number of boxes in the simulation.
    */
    const static unsigned boxes = 2;

    /** Holds the box data. */
    Box boxData[boxes];

    /** Holds the current shot type. */
    ShotType currentShotType;

	float orientationLeft;
	float orientationRight;

	cyclone::Vector3 leftCamPos;
	cyclone::Vector3 leftCamLook;
	cyclone::Vector3 rightCamPos;
	cyclone::Vector3 rightCamLook;

	cyclone::Vector3 currentCamPos;
	cyclone::Vector3 currentCamLook;

	cyclone::Vector3 fromPos;
	cyclone::Vector3 toPos;
	cyclone::Vector3 fromLook;
	cyclone::Vector3 toLook;

	cyclone::Vector3 leftPlayerPos;
	cyclone::Vector3 rightPlayerPos;

	float currentTransitionTime;
	float transitionTime;

	CameraPosState camPosState;
	CameraPosState lastPosState;

    /** Resets the position of all the boxes and primes the explosion. */
    virtual void reset();

    /** Build the contacts for the current situation. */
    virtual void generateContacts();

    /** Processes the objects in the simulation forward in time. */
    virtual void updateObjects(cyclone::real duration);

    /** Dispatches a round. */
    void fire();

public:
    /** Creates a new demo object. */
    BigBallisticDemo();

    /** Returns the window title for the demo. */
    virtual const char* getTitle();

    /** Sets up the rendering. */
    virtual void initGraphics();
    
    /** Display world. */
    virtual void display();

    /** Handle a mouse click. */
    virtual void mouse(int button, int state, int x, int y);

    /** Handle a keypress. */
    virtual void key(unsigned char key);
};

// Method definitions
BigBallisticDemo::BigBallisticDemo()
: 
RigidBodyApplication(),
currentShotType(PISTOL),
orientationLeft(0),
orientationRight(0)
{
    pauseSimulation = false;
    reset();
}


void BigBallisticDemo::initGraphics()
{
    GLfloat lightAmbient[] = {0.8f,0.8f,0.8f,1.0f};
    GLfloat lightDiffuse[] = {0.9f,0.95f,1.0f,1.0f};

    glLightfv(GL_LIGHT0, GL_AMBIENT, lightAmbient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, lightDiffuse);

    glEnable(GL_LIGHT0);

    Application::initGraphics();
}

void BigBallisticDemo::reset()
{
    // Make all shots unused
    for (AmmoRound *shot = ammo; shot < ammo+ammoRounds; shot++)
    {
        shot->type = UNUSED;
    }

	camPosState = CameraPosState::LEFT;
	lastPosState = CameraPosState::LEFT;

	currentTransitionTime = 0.0;
	transitionTime = 2.0;

	leftCamPos = cyclone::Vector3(-25.0, 8.0, 5.0);
	leftCamLook = cyclone::Vector3(0.0, 5.0, 22.0);

	rightCamPos = cyclone::Vector3(-25.0, 8.0, 95.0);
	rightCamLook = cyclone::Vector3(0.0, 5.0, 82);

	currentCamPos = leftCamPos;
	currentCamLook = leftCamLook;

	fromPos = cyclone::Vector3();
	toPos = cyclone::Vector3();
	fromLook = cyclone::Vector3();
	toLook = cyclone::Vector3();

    // Initialise the box
    cyclone::real z = 20.0f;
    /*for (Box *box = boxData; box < boxData+boxes; box++)
    {
        box->setState(z);
        z += 90.0f;
    }*/
	boxData[0].setState(-1.2);
	boxData[1].setState(100.2);
}

const char* BigBallisticDemo::getTitle()
{
    return "Cyclone > Big Ballistic Demo";
}

void BigBallisticDemo::fire()
{
    // Find the first available round.
    AmmoRound *shot;
    for (shot = ammo; shot < ammo+ammoRounds; shot++)
    {
        if (shot->type == UNUSED) break;
    }

    // If we didn't find a round, then exit - we can't fire.
    if (shot >= ammo+ammoRounds) return;

    // Set the shot
	shot->setState(currentShotType, orientationLeft, camPosState);

	camPosState = CameraPosState::IN_TRANSITION;

	switch (lastPosState)
	{
	case LEFT:
		fromPos = leftCamPos;
		toPos = rightCamPos;
		fromLook = leftCamLook;
		toLook = rightCamLook;
		break;
	case RIGHT:
		fromPos = rightCamPos;
		toPos = leftCamPos;
		fromLook = rightCamLook;
		toLook = leftCamLook;
		break;
	case IN_TRANSITION:
		break;
	default:
		break;
	}
}

void BigBallisticDemo::updateObjects(cyclone::real duration)
{
    // Update the physics of each particle in turn
    for (AmmoRound *shot = ammo; shot < ammo+ammoRounds; shot++)
    {
        if (shot->type != UNUSED)
        {
            // Run the physics
            shot->body->integrate(duration);
            shot->calculateInternals();

            // Check if the particle is now invalid
            if (shot->body->getPosition().y < 0.0f ||
                shot->startTime+5000 < TimingData::get().lastFrameTimestamp ||
                shot->body->getPosition().z > 200.0f)
            {
                // We simply set the shot type to be unused, so the
                // memory it occupies can be reused by another shot.
                shot->type = UNUSED;
            }
        }
    }

    // Update the boxes
    for (Box *box = boxData; box < boxData+boxes; box++)
    {
        // Run the physics
        box->body->integrate(duration);
        box->calculateInternals();
    }

	if (camPosState == CameraPosState::IN_TRANSITION)
	{
		currentTransitionTime += duration;

		cyclone::Vector3 posDiff = toPos - fromPos;
		cyclone::Vector3 lookDiff = toLook - fromLook;

		currentCamPos = fromPos + posDiff * (currentTransitionTime / transitionTime);
		currentCamLook = fromLook + lookDiff * (currentTransitionTime / transitionTime);

		if (currentTransitionTime >= transitionTime)
		{
			currentTransitionTime = 0.0;

			switch (lastPosState)
			{
			case LEFT:
				camPosState = CameraPosState::RIGHT;
				lastPosState = camPosState;
				break;
			case RIGHT:
				camPosState = CameraPosState::LEFT;
				lastPosState = camPosState;
				break;
			case IN_TRANSITION:
				break;
			default:
				break;
			}
		}
	}
}

void BigBallisticDemo::display()
{
    const static GLfloat lightPosition[] = {-1,1,0,0};
    
    // Clear the viewport and set the camera direction
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();
	gluLookAt(currentCamPos.x, currentCamPos.y, currentCamPos.z, currentCamLook.x, currentCamLook.y, currentCamLook.z,  0.0, 1.0, 0.0);

    // Draw a sphere at the firing point, and add a shadow projected
    // onto the ground plane.
    glColor3f(0.0f, 0.0f, 0.0f);
    glPushMatrix();
    glTranslatef(0.0f, 1.5f, 0.0f);
    glutSolidSphere(0.1f, 5, 5);

	glPushMatrix();
	switch (camPosState)
	{
	case LEFT:
		glRotatef(orientationLeft * -1.0f, 1, 0, 0);
		glBegin(GL_LINES);
		glVertex3f(0, 0, 0);
		glVertex3f(0, 0, 10);
		break;
	case RIGHT:
		glTranslatef(0.0f, 0.0, 99);
		glRotatef(orientationRight * -1.0f, 1, 0, 0);
		glBegin(GL_LINES);
		glVertex3f(0, 0, 0);
		glVertex3f(0, 0, -10);
		break;
	}
	glEnd();
	glPopMatrix();

    glTranslatef(0.0f, -1.5f, 0.0f);
    glColor3f(0.75f, 0.75f, 0.75f);
    glScalef(1.0f, 0.1f, 1.0f);
    glutSolidSphere(0.1f, 5, 5);
    glPopMatrix();

    // Draw some scale lines
    glColor3f(0.75f, 0.75f, 0.75f);
    glBegin(GL_LINES);
    for (unsigned i = 0; i < 200; i += 10)
    {
        glVertex3f(-5.0f, 0.0f, i);
        glVertex3f(5.0f, 0.0f, i);
    }
    glEnd();

    // Render each particle in turn
    glColor3f(1,0,0);
    for (AmmoRound *shot = ammo; shot < ammo+ammoRounds; shot++)
    {
        if (shot->type != UNUSED)
        {
            shot->render();
        }
    }

    // Render the box
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glLightfv(GL_LIGHT0, GL_POSITION, lightPosition);
    glColorMaterial(GL_FRONT_AND_BACK, GL_DIFFUSE);
    glEnable(GL_COLOR_MATERIAL);
    for (Box *box = boxData; box < boxData+boxes; box++)
    {
		glColor3f(box->color.x,box->color.y,box->color.z);
        box->render();
    }
    glDisable(GL_COLOR_MATERIAL);
    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);

    // Render the description
    glColor3f(0.0f, 0.0f, 0.0f);

	char angle[10];
	switch (camPosState)
	{
	case LEFT:
		sprintf(angle, "%f", orientationLeft);
		renderText(10.0f, 50.0f, angle);
		break;
	case RIGHT:
		sprintf(angle, "%f", orientationRight * -1.0f);
		renderText(10.0f, 50.0f, angle);
		break;
	case IN_TRANSITION:
		renderText(10.0f, 50.0f, "IN TRANSITION");
		break;
	}

    renderText(10.0f, 34.0f, "Click: Fire\n1-4: Select Ammo");

    // Render the name of the current shot type
    switch(currentShotType)
    {
    case PISTOL: renderText(10.0f, 10.0f, "Current Ammo: Pistol"); break;
    case ARTILLERY: renderText(10.0f, 10.0f, "Current Ammo: Artillery"); break;
    case FIREBALL: renderText(10.0f, 10.0f, "Current Ammo: Fireball"); break;
    case LASER: renderText(10.0f, 10.0f, "Current Ammo: Laser"); break;
    }
}

void BigBallisticDemo::generateContacts()
{
    // Create the ground plane data
    cyclone::CollisionPlane plane;
    plane.direction = cyclone::Vector3(0,1,0);
    plane.offset = 0;

    // Set up the collision data structure
    cData.reset(maxContacts);
    cData.friction = (cyclone::real)0.9;
    cData.restitution = (cyclone::real)0.1;
    cData.tolerance = (cyclone::real)0.1;

    // Check ground plane collisions
    for (Box *box = boxData; box < boxData+boxes; box++)
    {
        if (!cData.hasMoreContacts()) return;
        cyclone::CollisionDetector::boxAndHalfSpace(*box, plane, &cData);


        // Check for collisions with each shot
        for (AmmoRound *shot = ammo; shot < ammo+ammoRounds; shot++)
        {
            if (shot->type != UNUSED)
            {
                if (!cData.hasMoreContacts()) return;

                // When we get a collision, remove the shot
                if (cyclone::CollisionDetector::boxAndSphere(*box, *shot, &cData))
                {
                    shot->type = UNUSED;
					box->color = cyclone::Vector3(0, 0, 1);
                }
            }
        }
    }

    // NB We aren't checking box-box collisions.
}

void BigBallisticDemo::mouse(int button, int state, int x, int y)
{
    // Fire the current weapon.
    if (state == GLUT_DOWN) fire();
}

void BigBallisticDemo::key(unsigned char key)
{
    switch(key)
    {
    case '1': currentShotType = PISTOL; break;
    case '2': currentShotType = ARTILLERY; break;
    case '3': currentShotType = FIREBALL; break;
    case '4': currentShotType = LASER; break;

    case 'r': case 'R': reset(); break;

	case 'w':
		{
			if (camPosState == CameraPosState::LEFT)
			{
				orientationLeft += 1.0f;
			}
			else if (camPosState == CameraPosState::RIGHT)
			{
				orientationRight -= 1.0f;
			}
		}
		break;

	case 's':
		{
			if (camPosState == CameraPosState::LEFT)
			{
				orientationLeft -= 1.0f;
			}
			else if (camPosState == CameraPosState::RIGHT)
			{
				orientationRight += 1.0f;
			}
		}
		break;
    }
}

/**
 * Called by the common demo framework to create an application
 * object (with new) and return a pointer.
 */
Application* getApplication()
{
    return new BigBallisticDemo();
}
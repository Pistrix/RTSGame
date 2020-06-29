#include "Character.h"
#include <iostream>
#include <math.h>

Character::Character(Rectangle Body,  const char* name, Camera2D* camera, std::map<Vector2,Chunk*,Vec2Compare>* map){
  this->body = Body;
  this->x = Body.x;
  this->y = Body.y;
  this-> name = name;
  this->camera = camera;
  this->map = map;
}

void Character::setSpeed(float newSpeed){
  this->speed = newSpeed;
}

void Character::updatePos(Camera2D* camera){
  if(isSelected){
    if(IsMouseButtonPressed(MOUSE_LEFT_BUTTON)){
      Vector2 mPos = GetMousePosition();
      mPos = GetScreenToWorld2D(mPos,*camera);
      this->x = (float)((int)mPos.x - ((int)mPos.x % 20)); //Should be ChunkLength
      this->y = (float)((int)mPos.y - ((int)mPos.y % 20));
      if(map->find(Vector2{(float)this->x,(float)this->y})->second->isBlocked() == true){
	this->m = (this->y - this->body.y)/(this->x - this->body.x);
	this->b = -1*this->m*this->body.x +this->body.y;
	do{
	  if(this->x > this->body.x){
	    this->x = this->x - 20; //chunkLength
	    this->y = (float)((int)gety(this->x) - ((int)gety(this->x) % 20));
	    }
	}while(map->find(Vector2{(float)this->x,(float)this->y})->second->isBlocked() == true);
      }
      isMoving = true;
    }   
  }
  if(isMoving)
    moveToPoint(this->x,this->y);
}

void Character::updateStatus(Camera2D* camera){
  if(IsMouseButtonPressed(MOUSE_LEFT_BUTTON)){
    Vector2 mPos = GetMousePosition();
    mPos = GetScreenToWorld2D(mPos,*camera);
    int mx = (int)mPos.x;
    int my = (int)mPos.y;
    if(mx >= body.x && mx <= body.x+body.width && my >= body.y && my <= body.y+body.height){
      this->isSelected = true;
    }
    
  }
  else if(IsMouseButtonPressed(MOUSE_RIGHT_BUTTON)){
    this->isSelected = false;
  }
}

void Character::moveToPoint(int x, int y){
  if(isMoving){
    if(abs(body.x-x) < speed*GetFrameTime()) //if "close enough", set equal and stop moving"
      body.x = x;
    if(abs(body.y-y) < speed*GetFrameTime())
      body.y = y;
    if(body.y == y && body.x == x){
      isMoving = false;
      return;
    }
    if(body.x < x)
      body.x = body.x+speed*GetFrameTime();
    if(body.x > x)
      body.x = body.x-speed*GetFrameTime();
    if(body.y < y)
      body.y = body.y+speed*GetFrameTime();
    if(body.y > y)
      body.y = body.y-speed*GetFrameTime();
  }
}

void Character::draw(){DrawRectanglePro(body, Vector2{0,0},0.0f,BLACK);}
float Character::getX(){return body.x;}
float Character::getY(){return body.y;}
float Character::getWidth(){return body.width;}
float Character::getHeight(){return body.height;}
float Character::gety(float x){return this->m*x + this->b;}

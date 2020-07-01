#include "Character.h"
#include <iostream>
#include <math.h>

int Character::numSelected{0};
ChunkType Character::MCP_type{null};
Vector2 Character::mouseClickPoint;

Character::Character(Rectangle Body,  const char* name, Camera2D* camera, std::map<Vector2,Chunk*,Vec2Compare>* map){
  this->body = Body;
  this-> name = name;
  this->camera = camera;
  this->map = map;
  for(int i = modChunkLength(body.x); i < modChunkLength(body.x+body.width); i = i+chunkLength){
    for(int j = modChunkLength(body.y); j < modChunkLength(body.y+body.height); j = j+chunkLength){
      map->find(Vector2{(float)i,(float)j})->second->setChunkType(unitSpace);
    }
  }
}
/*
  if(selected)
      if(currPoint != travelPoint) then moveTowardsPoint
      if(isAttacking and in range yadda yadda) then attack
      travelPoint should be set by updateStatus when the following conditions occur
      Unit is selected, clickEventOccurs
          case 1: clickPoint is empty on map
	  case 2: clickPoint is obstructed by environment variable (find nearest open location)
	  case 3: clickPoint is another friendly unit
	  case 4: clickPoint is an enemy or enemy structure (not to be dealt with yet)
        
*/
void Character::updateUnit(Camera2D* camera){ // Interprets mouse action and updates unit accordingly
  currV = GetMousePosition();
  switch(markSet){
  case false: 
    if(IsMouseButtonPressed(MOUSE_LEFT_BUTTON)){
      x_init = currV.x; //Marking initial click point
      y_init = currV.y;
      markSet = true;
    }
    break;
  case true:
    if(!regionActive){
      if(abs(x_init-currV.x) > 2*chunkLength || abs(y_init- currV.y) > 2*chunkLength){
	regionActive = true;
      }
    }
    else{
      //Deselecting all units
      this->deselect();
      if(IsMouseButtonDown(MOUSE_LEFT_BUTTON)){
	currV = GetMousePosition();
	switch(Vec2Quad(Vector2{currV.x-x_init,currV.y-y_init})){
	case 1:
	  region = Rectangle{(float)x_init, (float)y_init, currV.x - x_init, currV.y - y_init};
	  DrawRectangleLinesEx(region,(int)(chunkLength/5), BLACK);
	  break;
	case 2:
	  region = Rectangle{currV.x, (float)y_init, x_init-currV.x, currV.y-y_init};
	  DrawRectangleLinesEx(region ,(int)(chunkLength/5), BLACK);
	  break;
	case 3:
	  region = Rectangle{currV.x, currV.y, x_init-currV.x, y_init-currV.y}; 
	  DrawRectangleLinesEx(region ,(int)(chunkLength/5), BLACK);
	  break;
	case 4:
	  region = Rectangle{(float)x_init, currV.y, currV.x-x_init, y_init-currV.y};
	  DrawRectangleLinesEx(region ,(int)(chunkLength/5), BLACK);
	  break;
	};
      }
    }
    break;
  }
  if(IsMouseButtonReleased(MOUSE_LEFT_BUTTON)){
    if(markSet == true && regionActive == true){ //Click and Drag Detection
      Vector2 topLeft =  GetScreenToWorld2D(Vector2{region.x,region.y},*camera);
      Vector2 botRight = GetScreenToWorld2D(Vector2{region.x+region.width,region.y+region.height},*camera);
      if(this->getX() >= topLeft.x && this->getX() <= botRight.x && this->getY() >= topLeft.y && this->getY() <= botRight.y){
	this->select();
      }
    }
    else if(markSet == true && regionActive == false){ // Click Detection
      Vector2 mPos = GetScreenToWorld2D(currV,*camera);
      mouseClickPoint.x = mPos.x;
      mouseClickPoint.y = mPos.y;
      analyzeMCP();
      if(MCP_type == unitSpace){
	Chunk* chunk = map->find(Vector2{modChunkLength(mouseClickPoint.x),modChunkLength(mouseClickPoint.y)})->second;
	if(chunk->getRect().x == modChunkLength(getX()) && chunk->getRect().y == modChunkLength(getY()))
	  this->select();
	MCP_type = null;
      }
      else if(MCP_type == freeSpace || MCP_type == structSpace){ // Find closest actual free space pos.
	if(this->selected == true){
	  findNearestFreeChunk();
	  isMoving = true;
	}
	MCP_type = null;
      }
    }  
    markSet = false;
    regionActive = false;
  }
  if(IsMouseButtonPressed(MOUSE_RIGHT_BUTTON)){ // Check for deselect request
    if(this->selected == true)
      this->deselect();
  }
  if(path.size())
    moveAlongPath();
}

// TODO: Make a better system for this, causes seg fault when moving multiple units near border
void Character::findNearestFreeChunk(){ // Finds nearest open space and sets destinX and destinY to that location
  Vector2 mPos = GetScreenToWorld2D(GetMousePosition(),*camera);
  int dirX = this->body.x - mPos.x;
  int dirY = this->body.y - mPos.y;
  dirX = (dirX > 0) - (dirX < 0);
  dirY = (dirY > 0) - (dirY < 0);
  destinX = mPos.x - (dirX*displacement*chunkLength);
  destinY = mPos.y - (dirY*displacement*chunkLength);
  while(map->find(Vector2{modChunkLength(destinX),modChunkLength(destinY)})->second->isBlocked() == true){
    if(modChunkLength(destinX) != modChunkLength(this->body.x)){ 
      this->m = (destinY - this->body.y)/(destinX - this->body.x);
      this->b = -1*this->m*this->body.x +this->body.y;
      if(destinX > this->body.x)
	destinX = destinX - (int)(chunkLength/4);
      else if(destinX < this->body.x)
	destinX = destinX + (int)(chunkLength/4);
      destinY = gety(destinX);
    }
    else{
      if(destinY > this->body.y)
	destinY = destinY - chunkLength;
      else
	destinY = destinY + chunkLength;
    }
  }
  destinX = modChunkLength(destinX);
  destinY = modChunkLength(destinY);
  path.clear();
  findPath();
}

void Character::moveAlongPath(){ // Incements player.pos towards next point on path
  Vector2 nextPoint = path.front();
  float x = nextPoint.x;
  float y = nextPoint.y;
  map->find(Vector2{modChunkLength(body.x),modChunkLength(body.y)})->second->setChunkType(ChunkType::freeSpace);
  if(abs(body.x-x) <= speed*GetFrameTime()) //if close enough then set equal and stop moving"
    body.x = x;
  if(abs(body.y-y) <= speed*GetFrameTime())
    body.y = y;
  if(body.y == y && body.x == x){
    path.pop_front();
    map->find(Vector2{modChunkLength(body.x),modChunkLength(body.y)})->second->setChunkType(ChunkType::unitSpace);
    return;
  }
  else{ // if far enough then increment
    if(body.x < x)
      body.x = body.x+speed*GetFrameTime();
    if(body.x > x)
      body.x = body.x-speed*GetFrameTime();
    if(body.y < y)
      body.y = body.y+speed*GetFrameTime();
    if(body.y > y)
      body.y = body.y-speed*GetFrameTime();
  }
  map->find(Vector2{modChunkLength(body.x),modChunkLength(body.y)})->second->setChunkType(ChunkType::unitSpace);
}

void Character::findPath(){ //A* Search path finding
  destinX = destinX/20;
  destinY = destinY/20;
  Vector2 src = Vector2{modChunkLength(body.x)/20,modChunkLength(body.y)/20};
  if(isDestination(src.y,src.x)){ // Should never happen
    return;
  }
  
  bool closedList[100][100]; // Make dynamic
  std::memset(closedList, false, sizeof(closedList));
  cell cellDetails[100][100];
  int i,j;

  for(i = 0; i < 100; i++){
    for(j = 0; j < 100; j++){
      cellDetails[i][j].f = FLT_MAX;
      cellDetails[i][j].g = FLT_MAX;
      cellDetails[i][j].h = FLT_MAX;
      cellDetails[i][j].parent_i = -1;
      cellDetails[i][j].parent_j = -1;
    }
  }
  // Initializing Source Node
  i = src.y;
  j = src.x;
  cellDetails[i][j].f = 0;
  cellDetails[i][j].g = 0;
  cellDetails[i][j].h = 0;
  cellDetails[i][j].parent_i = i;
  cellDetails[i][j].parent_j = j;

  std::set<pPair> openList;
  openList.insert(std::make_pair(0.0f,std::make_pair(i,j)));

  while(!openList.empty()){
    pPair p = *openList.begin();
    openList.erase(openList.begin());

    i = p.second.first; 
    j = p.second.second; 
    closedList[i][j] = true;

    float gNew, hNew, fNew;

    //----------- 1st Successor (North) ------------  
    if (isValid(i-1, j) == true) 
      { 
	if (isDestination(i-1, j) == true) 
	  { 
	    // Set the Parent of the destination cell 
	    cellDetails[i-1][j].parent_i = i; 
	    cellDetails[i-1][j].parent_j = j;  
	    setPath(cellDetails);
	    return; 
	  } 
	else if (closedList[i-1][j] == false && isUnBlocked(i-1, j) == true) 
	  { 
	    gNew = cellDetails[i][j].g + 1.0; 
	    hNew = computeH (i-1, j); 
	    fNew = gNew + hNew; 
	    if (cellDetails[i-1][j].f == FLT_MAX || 
		cellDetails[i-1][j].f > fNew) 
	      { 
		openList.insert(std::make_pair(fNew, std::make_pair(i-1,j)));  
		cellDetails[i-1][j].f = fNew; 
		cellDetails[i-1][j].g = gNew; 
		cellDetails[i-1][j].h = hNew; 
		cellDetails[i-1][j].parent_i = i; 
		cellDetails[i-1][j].parent_j = j; 
	      } 
	  } 
      }
    //----------- 2nd Successor (South) ------------  
    if (isValid(i+1, j) == true) 
      { 
	if (isDestination(i+1, j) == true) 
	  { 
	    // Set the Parent of the destination cell 
	    cellDetails[i+1][j].parent_i = i; 
	    cellDetails[i+1][j].parent_j = j;  
	    setPath(cellDetails);
	    return; 
	  } 
	else if (closedList[i+1][j] == false && isUnBlocked(i+1, j) == true) 
	  { 
	    gNew = cellDetails[i][j].g + 1.0; 
	    hNew = computeH (i+1, j); 
	    fNew = gNew + hNew; 
	    if (cellDetails[i+1][j].f == FLT_MAX || 
		cellDetails[i+1][j].f > fNew) 
	      { 
		openList.insert(std::make_pair(fNew, std::make_pair(i+1,j)));  
		cellDetails[i+1][j].f = fNew; 
		cellDetails[i+1][j].g = gNew; 
		cellDetails[i+1][j].h = hNew; 
		cellDetails[i+1][j].parent_i = i; 
		cellDetails[i+1][j].parent_j = j; 
	      } 
	  } 
      }
    //----------- 3rd Successor (East) ------------  
    if (isValid(i, j+1) == true) 
      { 
	if (isDestination(i, j+1) == true) 
	  { 
	    // Set the Parent of the destination cell 
	    cellDetails[i][j+1].parent_i = i; 
	    cellDetails[i][j+1].parent_j = j;  
	    setPath(cellDetails);
	    return; 
	  } 
	else if (closedList[i][j+1] == false &&  isUnBlocked(i, j+1) == true) 
	  { 
	    gNew = cellDetails[i][j].g + 1.0; 
	    hNew = computeH (i, j+1); 
	    fNew = gNew + hNew; 
	    if (cellDetails[i][j+1].f == FLT_MAX || 
		cellDetails[i][j+1].f > fNew) 
	      { 
		openList.insert(std::make_pair(fNew, std::make_pair(i,j+1)));  
		cellDetails[i][j+1].f = fNew; 
		cellDetails[i][j+1].g = gNew; 
		cellDetails[i][j+1].h = hNew; 
		cellDetails[i][j+1].parent_i = i; 
		cellDetails[i][j+1].parent_j = j; 
	      } 
	  } 
      }
    //----------- 4th Successor (West) ------------ 
    // Only process this cell if this is a valid one 
    if (isValid(i, j-1) == true) 
      { 
	if (isDestination(i, j-1) == true) 
	  { 
	    // Set the Parent of the destination cell 
	    cellDetails[i][j-1].parent_i = i; 
	    cellDetails[i][j-1].parent_j = j;  
	    setPath(cellDetails);
	    return; 
	  } 
	else if (closedList[i][j-1] == false && isUnBlocked(i, j-1) == true) 
	  { 
	    gNew = cellDetails[i][j].g + 1.0; 
	    hNew = computeH(i, j-1); 
	    fNew = gNew + hNew; 
	    if (cellDetails[i][j-1].f == FLT_MAX || 
		cellDetails[i][j-1].f > fNew) 
	      { 
		openList.insert( std::make_pair (fNew, std::make_pair (i, j-1))); 
		// Update the details of this cell 
		cellDetails[i][j-1].f = fNew; 
		cellDetails[i][j-1].g = gNew; 
		cellDetails[i][j-1].h = hNew; 
		cellDetails[i][j-1].parent_i = i; 
		cellDetails[i][j-1].parent_j = j; 
	      } 
	  } 
      }
    //----------- 5th Successor (NE) ------------ 
    // Only process this cell if this is a valid one 
    if (isValid(i-1, j+1) == true) 
      { 
	if (isDestination(i-1, j+1) == true && isUnBlocked(i,j+1) == true && isUnBlocked(i-1,j) == true) 
	  { 
	    // Set the Parent of the destination cell 
	    cellDetails[i-1][j+1].parent_i = i; 
	    cellDetails[i-1][j+1].parent_j = j;  
	    setPath(cellDetails);
	    return; 
	  } 
	else if (closedList[i-1][j+1] == false && isUnBlocked(i-1, j+1) == true && isUnBlocked(i,j+1) == true && isUnBlocked(i-1,j) == true) 
	  { 
	    gNew = cellDetails[i][j].g + 1.0; 
	    hNew = computeH(i-1, j+1); 
	    fNew = gNew + hNew; 
	    if (cellDetails[i-1][j+1].f == FLT_MAX || 
		cellDetails[i-1][j+1].f > fNew) 
	      { 
		openList.insert( std::make_pair (fNew, std::make_pair (i-1, j+1))); 
		// Update the details of this cell 
		cellDetails[i-1][j+1].f = fNew; 
		cellDetails[i-1][j+1].g = gNew; 
		cellDetails[i-1][j+1].h = hNew; 
		cellDetails[i-1][j+1].parent_i = i; 
		cellDetails[i-1][j+1].parent_j = j; 
	      } 
	  } 
      }
    //----------- 6th Successor (SE) ------------ 
    // Only process this cell if this is a valid one 
    if (isValid(i+1, j+1) == true) 
      { 
	if (isDestination(i+1, j+1) == true && isUnBlocked(i,j+1) == true && isUnBlocked(i+1,j) == true) 
	  { 
	    // Set the Parent of the destination cell 
	    cellDetails[i+1][j+1].parent_i = i; 
	    cellDetails[i+1][j+1].parent_j = j;  
	    setPath(cellDetails);
	    return; 
	  } 
	else if (closedList[i+1][j+1] == false && isUnBlocked(i+1, j+1) == true && isUnBlocked(i,j+1) == true && isUnBlocked(i+1,j) == true) 
	  { 
	    gNew = cellDetails[i][j].g + 1.0; 
	    hNew = computeH(i+1, j+1); 
	    fNew = gNew + hNew; 
	    if (cellDetails[i+1][j+1].f == FLT_MAX || 
		cellDetails[i+1][j+1].f > fNew) 
	      { 
		openList.insert( std::make_pair (fNew, std::make_pair (i+1, j+1))); 
		// Update the details of this cell 
		cellDetails[i+1][j+1].f = fNew; 
		cellDetails[i+1][j+1].g = gNew; 
		cellDetails[i+1][j+1].h = hNew; 
		cellDetails[i+1][j+1].parent_i = i; 
		cellDetails[i+1][j+1].parent_j = j; 
	      } 
	  } 
      }
    //----------- 7th Successor (SW) ------------ 
    // Only process this cell if this is a valid one 
    if (isValid(i+1, j-1) == true) 
      { 
	if (isDestination(i+1, j-1) == true && isUnBlocked(i,j-1) == true && isUnBlocked(i+1,j) == true) 
	  { 
	    // Set the Parent of the destination cell 
	    cellDetails[i+1][j-1].parent_i = i; 
	    cellDetails[i+1][j-1].parent_j = j;  
	    setPath(cellDetails);
	    return; 
	  } 
	else if (closedList[i+1][j-1] == false && isUnBlocked(i+1, j-1) == true && isUnBlocked(i,j-1) == true && isUnBlocked(i+1,j) == true) 
	  { 
	    gNew = cellDetails[i][j].g + 1.0; 
	    hNew = computeH(i+1, j-1); 
	    fNew = gNew + hNew; 
	    if (cellDetails[i+1][j-1].f == FLT_MAX || 
		cellDetails[i+1][j-1].f > fNew) 
	      { 
		openList.insert( std::make_pair (fNew, std::make_pair (i+1, j-1))); 
		// Update the details of this cell 
		cellDetails[i+1][j-1].f = fNew; 
		cellDetails[i+1][j-1].g = gNew; 
		cellDetails[i+1][j-1].h = hNew; 
		cellDetails[i+1][j-1].parent_i = i; 
		cellDetails[i+1][j-1].parent_j = j; 
	      } 
	  } 
      }
    //----------- 8th Successor (NW) ------------ 
    // Only process this cell if this is a valid one 
    if (isValid(i-1, j-1) == true) 
      { 
	if (isDestination(i-1, j-1) == true && isUnBlocked(i,j-1) == true && isUnBlocked(i-1,j) == true) 
	  { 
	    // Set the Parent of the destination cell 
	    cellDetails[i-1][j-1].parent_i = i; 
	    cellDetails[i-1][j-1].parent_j = j;  
	    setPath(cellDetails);
	    return; 
	  } 
	else if (closedList[i-1][j-1] == false && isUnBlocked(i-1, j-1) == true && isUnBlocked(i,j-1) == true && isUnBlocked(i-1,j) == true) 
	  { 
	    gNew = cellDetails[i][j].g + 1.0; 
	    hNew = computeH(i-1, j-1); 
	    fNew = gNew + hNew; 
	    if (cellDetails[i-1][j-1].f == FLT_MAX || 
		cellDetails[i-1][j-1].f > fNew) 
	      { 
		openList.insert( std::make_pair (fNew, std::make_pair (i-1, j-1))); 
		// Update the details of this cell 
		cellDetails[i-1][j-1].f = fNew; 
		cellDetails[i-1][j-1].g = gNew; 
		cellDetails[i-1][j-1].h = hNew; 
		cellDetails[i-1][j-1].parent_i = i; 
		cellDetails[i-1][j-1].parent_j = j; 
	      } 
	  } 
      }
  }
}

void Character::setPath(cell cellDetails[][100]){
  int row = destinY; 
  int col = destinX; 
  
  std::stack<Pair> Path; 
  
  while (!(cellDetails[row][col].parent_i == row && cellDetails[row][col].parent_j == col )){ 
    Path.push (std::make_pair (row, col)); 
    int temp_row = cellDetails[row][col].parent_i; 
    int temp_col = cellDetails[row][col].parent_j; 
    row = temp_row; 
    col = temp_col; 
  } 
  Path.push (std::make_pair (row, col));
  Path.pop(); //Don't need to travel to starting point
  while (!Path.empty()){ 
    Pair p = Path.top();
    Vector2 temp = Vector2{(float)chunkLength*p.second,(float)chunkLength*p.first};
    this->path.push_back(temp);
    Path.pop(); 
  } 
  return; 
}

bool Character::isUnBlocked(int row, int col){
  return !map->find(Vector2{col*chunkLength,row*chunkLength})->second->isBlocked();
}

int Character::Vec2Quad(Vector2 v){ //Returns quadrant BR = 1, BL = 2, TL = 3, TR = 4
  if(v.x >= 0 && v.y >= 0)
    return 1;
  else if(v.x <= 0 && v.y >= 0)
    return 2;
  else if(v.x <= 0 && v.y <= 0)
    return 3;
  else
    return 4;
}

void Character::analyzeMCP(){
  auto it = map->cend();
  it--;
  if(mouseClickPoint.x < 0 || mouseClickPoint.y < 0 || mouseClickPoint.x > it->first.x + chunkLength || mouseClickPoint.y > it->first.y + chunkLength) //End of Map
    return;
  Chunk* chunk = map->find(Vector2{modChunkLength(mouseClickPoint.x),modChunkLength(mouseClickPoint.y)})->second;
  if(chunk->getChunkType() == unitSpace){;
    MCP_type = unitSpace;
  }
  else if(chunk->getChunkType() == freeSpace){
    MCP_type = freeSpace;
  }
  else if(chunk->getChunkType() == structSpace){
    MCP_type = structSpace;
  }
}

bool Character::isDestination(int row, int col){
  if(row == destinY && col == destinX)
    return true;
  return false;
}

bool Character::isValid(int row, int col){return   (row >= 0) && (row < 100) && (col >= 0) && (col < 100);}

float Character::computeH(int row, int col){
  return (float)sqrt((row-destinY)*(row-destinY) + (col-destinX)*(col-destinX));
}

bool Character::isSelected(){return selected;}

void Character::select(){if(this->selected == false){this->selected = true; displacement = numSelected; numSelected++;}}
void Character::deselect(){if(this->selected == true){this->selected = false; displacement = 0; numSelected--;}}
void Character::draw(){DrawRectanglePro(body, Vector2{0,0},0.0f,BLACK);}
void Character::setSpeed(float newSpeed){this->speed = newSpeed;}

float Character::gety(float x){return this->m*x + this->b;}
float Character::getHeight(){return body.height;}
float Character::getWidth(){return body.width;}
float Character::getX(){return body.x;}
float Character::getY(){return body.y;}

#include "VIPLimiter.h"
#define EPS (1e-8)

void useVIPLimiter(int neigh_cell_num, double** Vave, double* V0, double* Vp)
{
	bool colinear(true);
	int node_num(0),count(0);
	double area(0);
	std::vector<std::vector<double> > CH(2, std::vector<double>(2));

	//Temporally, we choose to do noting in these cases...
	if(neigh_cell_num != 3 && neigh_cell_num != 4)
		return;

	//Set initial guess for the CH...
	if(Vave[0][0] > Vave[1][0])
	{
		CH[0][0] = Vave[0][0];
		CH[0][1] = Vave[0][1];
		CH[1][0] = Vave[1][0];
		CH[1][1] = Vave[1][1];
	}
	else
	{
		CH[0][0] = Vave[1][0];
		CH[0][1] = Vave[1][1];
		CH[1][0] = Vave[0][0];
		CH[1][1] = Vave[0][1];
	}

	//Now, we try to find the initial triangle for the CH
	for(int e = 2; e < neigh_cell_num; ++e)
	{
		area = getTriArea(CH[0][0], CH[0][1], CH[1][0], CH[1][1], Vave[e][0], Vave[e][1]);

		if( fabs(area) < EPS ) //node[e] is colinear with node[0] and node[1]
		{
			if( Vave[e][0] > CH[0][0] )
			{
				CH[0][0] = Vave[e][0];
				CH[0][1] = Vave[e][1];
			}
			else if( Vave[e][0] < CH[1][0] )
			{
				CH[1][0] = Vave[e][0];
				CH[1][1] = Vave[e][1];
			}
			else
			{
				//do nothing here...
			}
		}
		else //build a triangle 
		{
			count = e+1;

			std::vector<double> new_node(2);
			new_node[0] = Vave[e][0];
			new_node[1] = Vave[e][1];

			if( area > 0 )
				CH.push_back(new_node);
			else
				CH.insert(CH.begin()+1,new_node);

			colinear = false;

			break;
		}
	}

	//Case-1: the given cell velocities are all colinear... 
	if(colinear) 
	{
		if ( insideSegment(CH[0][0], CH[0][1], CH[1][0], CH[1][1], V0[0], V0[1]) )
		{						
			area = getTriArea(CH[0][0], CH[0][1], CH[1][0], CH[1][1], Vp[0], Vp[1]);

			if ( fabs(area) > EPS )
			{
				getPerpendFoot(CH[0][0],CH[0][1],CH[1][0],CH[1][1],Vp[0],Vp[1],Vp);
			}

			if ( obtuseAngle(CH[0][0],CH[0][1], CH[1][0],CH[1][1], Vp[0],Vp[1]) )
			{
				Vp[0] = Vave[0][0];
				Vp[1] = Vave[0][1];
				return;
			}
			else if ( obtuseAngle(CH[1][0],CH[1][1], CH[0][0],CH[0][1], Vp[0],Vp[1]) )
			{
				Vp[0] = Vave[1][0];
				Vp[1] = Vave[1][1];
				return;
			}
			else
			{
				return;	
			}
		}
		else
		{
			Vp[0] = V0[0];
			Vp[1] = V0[1];
			return;
		}
	}

	//Case-2: check if the CH can be further extended using the last node...
	if( !colinear && count < neigh_cell_num )
	{
		bool face0(false), face1(false), face2(false);

		//face0: 1->2
		double n0x =   CH[2][1]-CH[1][1];
		double n0y = -(CH[2][0]-CH[1][0]);

		//face1: 2->0
		double n1x =   CH[0][1]-CH[2][1];
		double n1y = -(CH[0][0]-CH[2][0]);

		//face2: 0->1
		double n2x =   CH[1][1]-CH[0][1];
		double n2y = -(CH[1][0]-CH[0][0]);

		//check the node v.s. face position
		if ((Vave[count][0] - CH[1][0])*n0x + (Vave[count][1] - CH[1][1])*n0y > 0.0)
			face0 = true;

		if ((Vave[count][0] - CH[2][0])*n1x + (Vave[count][1] - CH[2][1])*n1y > 0.0)
			face1 = true;

		if ((Vave[count][0] - CH[0][0])*n1x + (Vave[count][1] - CH[0][1])*n1y > 0.0)
			face2 = true;

		//there are seven possible cases
		if (face0 && face1)
		{
			CH[2][0] = Vave[count][0];
			CH[2][1] = Vave[count][1];
		}
		else if (face0 && face2)
		{
			CH[1][0] = Vave[count][0];
			CH[1][1] = Vave[count][1];
		}
		else if (face1 && face2)
		{
			CH[0][0] = Vave[count][0];
			CH[0][1] = Vave[count][1];
		}
		else if (face0)
		{
			std::vector<double> new_node(2);
			new_node[0] = Vave[count][0];
			new_node[1] = Vave[count][1];
			CH.insert(CH.begin() + 2, new_node);
		}
		else if (face1)
		{
			std::vector<double> new_node(2);
			new_node[0] = Vave[count][0];
			new_node[1] = Vave[count][1];
			CH.insert(CH.begin() + 3, new_node);
		}
		else if (face2)
		{
			std::vector<double> new_node(2);
			new_node[0] = Vave[count][0];
			new_node[1] = Vave[count][1];
			CH.insert(CH.begin() + 1, new_node);
		}
		else
		{
			//do nothing here, the CH is not extended by the extra node
		}
	}

	//Now, we do the VIP limiter using the CH we find above...
	//(1) If V0 lies outside the CH, we set Vp=V0...
	//(2) If V0 lies inside the CH, then we check whether Vp lies inside the CH or not and limit Vp if necessary...

	if ( CH.size() == 3 )
	{
		if ( insideTriCH(CH, false, V0) )
		{
			insideTriCH(CH, true, Vp);
		}
		else
		{
			Vp[0] = V0[0];
			Vp[1] = V0[1];
		}
	}
	else if ( CH.size() == 4 )
	{
		if( insideQuadCH(CH, false, V0) )
		{
			insideQuadCH(CH, true, Vp);
		}
		else
		{
			Vp[0] = V0[0];
			Vp[1] = V0[1];
		}
	}
}

///////////////////////////////////////////////////
//some subroutines called by useViPLimiter...
///////////////////////////////////////////////////
double getTriArea(double x0, double y0, double x1, double y1, double xp, double yp)
{
	return (xp - x0)*(yp - y1) - (xp - x1)*(yp - y0);
}

void getPerpendFoot(double x0, double y0, double x1, double y1, double xc, double yc, double* pf)
{
	double k(0);

	k = ((xc-x0)*(x1-x0) + (yc-y0)*(y1-y0)) / ((x1-x0)*(x1-x0) + (y1-y0)*(y1-y0));

	pf[0] = x0 + k*(x1-x0);
	pf[1] = y0 + k*(y1-y0);
}

bool obtuseAngle(double x0, double y0, double xa, double ya, double xb, double yb)
{
	if( (xa-x0)*(xb-x0) + (ya-y0)*(yb-y0) > 0.0 )
		return false;
	else
		return true;
}

bool insideSegment(double x0, double y0, double x1, double y1, double xp, double yp)
{
	double area(0);

	area = getTriArea(x0, y0, x1, y1, xp, yp);

	if ( fabs(area) > EPS )
	{
		return false;
	}
	else if ( xp > std::max(x0,x1) || xp < std::min(x0,x1) )
	{
		return false;
	}

	return true;
}

bool insideTriCH(std::vector<std::vector<double> >& CH, bool flag, double* Vp)
{
	bool face0(false), face1(false), face2(false);

	//face0: 1->2
	double n0x =   CH[2][1] - CH[1][1];
	double n0y = -(CH[2][0] - CH[1][0]);

	//face1: 2->0
	double n1x =   CH[0][1] - CH[2][1];
	double n1y = -(CH[0][0] - CH[2][0]);

	//face2: 0->1
	double n2x =   CH[1][1] - CH[0][1];
	double n2y = -(CH[1][0] - CH[0][0]);

	//check the node v.s. face position
	if ((Vp[0] - CH[1][0])*n0x + (Vp[1] - CH[1][1])*n0y > 0.0)
		face0 = true;

	if ((Vp[0] - CH[2][0])*n1x + (Vp[1] - CH[2][1])*n1y > 0.0)
		face1 = true;

	if ((Vp[0] - CH[0][0])*n2x + (Vp[1] - CH[0][1])*n2y > 0.0)
		face2 = true;

	if (!flag)
	{
		if(face0 || face1 || face2)
			return false;
		else
			return true;
	}

	//if flag, we may need to limit Vp
	if (face0)
	{
		if(obtuseAngle(CH[2][0],CH[2][1], CH[1][0],CH[1][1], Vp[0],Vp[1]))
		{
			Vp[0] = CH[2][0];
			Vp[1] = CH[2][1];
		}
		else if(obtuseAngle(CH[1][0],CH[1][1], CH[2][0],CH[2][1], Vp[0],Vp[1]))
		{
			Vp[0] = CH[1][0];
			Vp[1] = CH[1][1];
		}
		else
		{
			getPerpendFoot(CH[1][0],CH[1][1],CH[2][0],CH[2][1],Vp[0],Vp[1],Vp);
		}

		return false;
	}
	else if (face1)
	{
		if(obtuseAngle(CH[2][0],CH[2][1], CH[0][0],CH[0][1], Vp[0],Vp[1]))
		{
			Vp[0] = CH[2][0];
			Vp[1] = CH[2][1];
		}
		else if(obtuseAngle(CH[0][0],CH[0][1], CH[2][0],CH[2][1], Vp[0],Vp[1]))
		{
			Vp[0] = CH[0][0];
			Vp[1] = CH[0][1];
		}
		else
		{
			getPerpendFoot(CH[2][0],CH[2][1],CH[0][0],CH[0][1],Vp[0],Vp[1],Vp);
		}

		return false;
	}
	else if (face2)
	{
		if(obtuseAngle(CH[0][0],CH[0][1], CH[1][0],CH[1][1], Vp[0],Vp[1]))
		{
			Vp[0] = CH[0][0];
			Vp[1] = CH[0][1];
		}
		else if(obtuseAngle(CH[1][0],CH[1][1], CH[0][0],CH[0][1], Vp[0],Vp[1]))
		{
			Vp[0] = CH[1][0];
			Vp[1] = CH[1][1];
		}
		else
		{
			getPerpendFoot(CH[0][0],CH[0][1],CH[1][0],CH[1][1],Vp[0],Vp[1],Vp);
		}

		return false;
	}
	else
	{
		//do nothing here...
	}

	return true;
}

bool insideQuadCH(std::vector<std::vector<double> >& CH, bool flag, double* Vp)
{
	bool face0(false), face1(false), face2(false), face3(false);

	//face0: 0->1
	double n0x =   CH[1][1] - CH[0][1];
	double n0y = -(CH[1][0] - CH[0][0]);

	//face1: 1->2
	double n1x =   CH[2][1] - CH[1][1];
	double n1y = -(CH[2][0] - CH[1][0]);

	//face2: 2->3
	double n2x =   CH[3][1] - CH[2][1];
	double n2y = -(CH[3][0] - CH[2][0]);

	//face3: 3->0
	double n3x =   CH[0][1] - CH[3][1];
	double n3y = -(CH[0][0] - CH[3][0]);

	//check the node v.s. face position
	if ((Vp[0] - CH[0][0])*n0x + (Vp[1] - CH[0][1])*n0y > 0.0)
		face0 = true;

	if ((Vp[0] - CH[1][0])*n1x + (Vp[1] - CH[1][1])*n1y > 0.0)
		face1 = true;

	if ((Vp[0] - CH[2][0])*n2x + (Vp[1] - CH[2][1])*n2y > 0.0)
		face2 = true;

	if ((Vp[0] - CH[3][0])*n3x + (Vp[1] - CH[3][1])*n3y > 0.0)
		face3 = true;

	if (!flag)
	{
		if(face0 || face1 || face2 || face3)
			return false;
		else
			return true;
	}

	//if flag, we may need to limit Vp
	if (face0)
	{
		if(obtuseAngle(CH[0][0],CH[0][1], CH[1][0],CH[1][1], Vp[0],Vp[1]))
		{
			Vp[0] = CH[0][0];
			Vp[1] = CH[0][1];
		}
		else if(obtuseAngle(CH[1][0],CH[1][1], CH[0][0],CH[0][1], Vp[0],Vp[1]))
		{
			Vp[0] = CH[1][0];
			Vp[1] = CH[1][1];
		}
		else
		{
			getPerpendFoot(CH[0][0],CH[0][1],CH[1][0],CH[1][1],Vp[0],Vp[1],Vp);
		}
		return false;
	}
	else if (face1)
	{
		if(obtuseAngle(CH[2][0],CH[2][1], CH[1][0],CH[1][1], Vp[0],Vp[1]))
		{
			Vp[0] = CH[2][0];
			Vp[1] = CH[2][1];
		}
		else if(obtuseAngle(CH[1][0],CH[1][1], CH[2][0],CH[2][1], Vp[0],Vp[1]))
		{
			Vp[0] = CH[1][0];
			Vp[1] = CH[1][1];
		}
		else
		{
			getPerpendFoot(CH[1][0],CH[1][1],CH[2][0],CH[2][1],Vp[0],Vp[1],Vp);
		}
		return false;
	}
	else if (face2)
	{
		if(obtuseAngle(CH[2][0],CH[2][1], CH[3][0],CH[3][1], Vp[0],Vp[1]))
		{
			Vp[0] = CH[2][0];
			Vp[1] = CH[2][1];
		}
		else if(obtuseAngle(CH[3][0],CH[3][1], CH[2][0],CH[2][1], Vp[0],Vp[1]))
		{
			Vp[0] = CH[3][0];
			Vp[1] = CH[3][1];
		}
		else
		{
			getPerpendFoot(CH[3][0],CH[3][1],CH[2][0],CH[2][1],Vp[0],Vp[1],Vp);
		}
		return false;
	}
	else if (face3)
	{
		if(obtuseAngle(CH[0][0],CH[0][1], CH[3][0],CH[3][1], Vp[0],Vp[1]))
		{
			Vp[0] = CH[0][0];
			Vp[1] = CH[0][1];
		}
		else if(obtuseAngle(CH[3][0],CH[3][1], CH[0][0],CH[0][1], Vp[0],Vp[1]))
		{
			Vp[0] = CH[3][0];
			Vp[1] = CH[3][1];
		}
		else
		{
			getPerpendFoot(CH[3][0],CH[3][1],CH[0][0],CH[0][1],Vp[0],Vp[1],Vp);
		}
		return false;
	}
	else
	{
		//do nothing here...
	}

	return true;
}
////////////////////////////////////////////////////

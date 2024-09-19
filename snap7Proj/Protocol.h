#pragma once
#include <QString>



struct stTagInfo
{
	QString strName;
	QString strType = "DB";
	int 	iNumber = 1;
	int 	iStart = 0;
	int 	iPosition = 0;
	QString strDataType = "bool";
	int		iSize = 0;
};



#pragma pack()
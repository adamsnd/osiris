/*
* ===========================================================================
*
*                            PUBLIC DOMAIN NOTICE
*               National Center for Biotechnology Information
*
*  This software/database is a "United States Government Work" under the
*  terms of the United States Copyright Act.  It was written as part of
*  the author's official duties as a United States Government employee and
*  thus cannot be copyrighted.  This software/database is freely available
*  to the public for use. The National Library of Medicine and the U.S.
*  Government have not placed any restriction on its use or reproduction.
*
*  Although all reasonable efforts have been taken to ensure the accuracy
*  and reliability of the software and data, the NLM and the U.S.
*  Government do not and cannot warrant the performance or results that
*  may be obtained by using this software or data. The NLM and the U.S.
*  Government disclaim all warranties, express or implied, including
*  warranties of performance, merchantability or fitness for any particular
*  purpose.
*
*  Please cite the author in any work or product based on this material.
*
* ===========================================================================
*
*  FileName: CoreBioComponent.cpp
*  Author:   Robert Goor
*
*/
//
//     class CoreBioComponent and other abstract base classes that represent samples and control sets of various kinds
//

#include "CoreBioComponent.h"
#include "rgfile.h"
#include "rgvstream.h"
#include "DataSignal.h"
#include "SampleData.h"
#include "ChannelData.h"
#include "rgtokenizer.h"
#include "GenotypeSpecs.h"
#include "Notices.h"
#include "IndividualGenotype.h"
#include "ParameterServer.h"
#include "ListFunctions.h"
#include "xmlwriter.h"
#include "SmartMessage.h"
#include "SmartNotice.h"
#include "STRSmartNotices.h"
#include "DirectoryManager.h"


Boolean CoreBioComponent::SearchByName = TRUE;
Boolean CoreBioComponent::GaussianSignature = TRUE;
Boolean CoreBioComponent::UseRawData = TRUE;
RGDList CoreBioComponent::testChannelArtifactNoticeList;
int CoreBioComponent::minBioIDForArtifacts = 0;
bool* CoreBioComponent::InitialMatrix = NULL;
bool* CoreBioComponent::OffScaleData = NULL;
int CoreBioComponent::OffScaleDataLength = 0;
double CoreBioComponent::minPrimaryPullupThreshold = 500.0;

bool CoreBioComponent::UseHermiteTimeTransforms = false;



ABSTRACT_DEFINITION (CoreBioComponent)

/*
ABSTRACT_DEFINITION (KnownBioSample)
ABSTRACT_DEFINITION (NegativeControl)
ABSTRACT_DEFINITION (PositiveControl)
ABSTRACT_DEFINITION (OtherKnownSample)
ABSTRACT_DEFINITION (ControlGrid)
ABSTRACT_DEFINITION (UnknownBioSample)*/


GridDataStruct :: GridDataStruct (PopulationCollection* collection, const RGString& markerSetName, TestCharacteristic* testPeak,
	RGTextOutput& text, RGTextOutput& ExcelText, OsirisMsg& msg, Boolean print) :
mCollection (collection),
mMarkerSetName (markerSetName),
mTestControlPeak (testPeak),
mText (text),
mExcelText (ExcelText),
mMsg (msg),
mPrint (print) {

}


SampleDataStruct :: SampleDataStruct (PopulationCollection* collection, const RGString& markerSetName, TestCharacteristic* testControlPeak,
	TestCharacteristic* testSamplePeak, RGTextOutput& text, RGTextOutput& ExcelText, OsirisMsg& msg, Boolean print) :
mCollection (collection),
mMarkerSetName (markerSetName),
mTestControlPeak (testControlPeak),
mTestSamplePeak (testSamplePeak),
mText (text),
mExcelText (ExcelText),
mMsg (msg),
mPrint (print) {

}



CoreBioComponent :: CoreBioComponent () : SmartMessagingObject (), mDataChannels (NULL), mNumberOfChannels (-1), mMarkerSet (NULL), 
mLSData (NULL), mLaneStandard (NULL), mAssociatedGrid (NULL) {

	InitializeSmartMessages ();
}


CoreBioComponent :: CoreBioComponent (const RGString& name) : SmartMessagingObject (), mName (name), 
mDataChannels (NULL), mNumberOfChannels (-1), mMarkerSet (NULL), mLSData (NULL), mLaneStandard (NULL), 
mAssociatedGrid (NULL) {

	InitializeSmartMessages ();
}


CoreBioComponent :: CoreBioComponent (const CoreBioComponent& component) : SmartMessagingObject ((SmartMessagingObject&) component),
mName (component.mName), mSampleName (component.mSampleName), mTime (component.mTime), mDate (component.mDate), mDataChannels (NULL), mNumberOfChannels (component.mNumberOfChannels), 
mMarkerSet (NULL), mLaneStandardChannel (component.mLaneStandardChannel), mTest (NULL), mLSData (NULL), mLaneStandard (NULL), 
mAssociatedGrid (component.mAssociatedGrid) {

	InitializeSmartMessages (component);
}


CoreBioComponent :: CoreBioComponent (const CoreBioComponent& component, CoordinateTransform* trans)  : SmartMessagingObject ((SmartMessagingObject&) component),
mName (component.mName), mSampleName (component.mSampleName), mTime (component.mTime), mDate (component.mDate), mDataChannels (NULL), mNumberOfChannels (component.mNumberOfChannels), 
mMarkerSet (NULL), mLaneStandardChannel (component.mLaneStandardChannel), mTest (NULL), mLSData (NULL), mLaneStandard (NULL), 
mAssociatedGrid (component.mAssociatedGrid) {

	mDataChannels = new ChannelData* [mNumberOfChannels + 1];

	for (int i=1; i<=mNumberOfChannels; i++) {

		if (i == mLaneStandardChannel)
			mDataChannels [i] = NULL;

		else {

			mDataChannels [i] = component.mDataChannels [i]->CreateNewTransformedChannel (*component.mDataChannels [i], trans);
			mChannelList.Append (mDataChannels [i]);
		}
	}

	InitializeSmartMessages (component);
}


CoreBioComponent :: ~CoreBioComponent () {

	mChannelList.Clear ();

	if (mDataChannels != NULL) {

		for (int i=1; i<=mNumberOfChannels; i++)
			delete mDataChannels [i];

		delete[] mDataChannels;
	}

	delete mMarkerSet;
	NewNoticeList.ClearAndDelete ();
	delete mAssociatedGrid;

	list<CompoundSignalInfo*>::const_iterator c1Iterator;
	CompoundSignalInfo* nextLink;

	for (c1Iterator = mSignalLinkList.begin (); c1Iterator != mSignalLinkList.end (); c1Iterator++) {

		nextLink = *c1Iterator;
		delete nextLink;
	}

	mSignalLinkList.clear ();

	InterchannelLinkage* nextInterchannelLinkage;
	list<InterchannelLinkage*>::const_iterator c2Iterator;

	for (c2Iterator = mInterchannelLinkageList.begin (); c2Iterator != mInterchannelLinkageList.end (); c2Iterator++) {

		nextInterchannelLinkage = *c2Iterator;
		delete nextInterchannelLinkage;
	}

	mInterchannelLinkageList.clear ();
	CoreBioComponent::ReleaseOffScaleData ();
}



void CoreBioComponent :: SetName (const RGString& name) {

	mName = name;
}


void CoreBioComponent :: SetTime (const RGString& time) {

	mTime = time;
}


void CoreBioComponent :: SetTime (const PackedTime& time) {

	mTime = time;
}


void CoreBioComponent :: SetTableLink (int linkNumber) {

	RGString temp;
	temp.Convert (linkNumber, 10);
	mTableLink = "&" + temp + "&";

	for (int i=1; i<=mNumberOfChannels; i++)
		mDataChannels [i]->SetTableLink (linkNumber);
}


int CoreBioComponent :: TimeSeparation (const CoreBioComponent* cbc) {

	return mTime.Distance (cbc->mTime);
}


Locus* CoreBioComponent :: FindLocus (int channel, const RGString& locusName) {

	return mDataChannels [channel]->FindLocus (locusName);
}


Locus* CoreBioComponent :: FindLocus (const RGString& locusName) {

	Locus* locus = mMarkerSet->FindLocus (locusName);

	if (locus == NULL)
		return NULL;

	return FindLocus (locus->GetLocusChannel (), locusName);
}


double CoreBioComponent :: GetTimeForSpecifiedID (int channel, double id) {

	return mDataChannels [channel]->GetTimeForSpecifiedID (id);
}


int CoreBioComponent :: CreateAndSubstituteFilteredDataSignalForRawDataNonILS (int window) {

	int i;

	for (i=1; i<=mNumberOfChannels; i++) {

		if (i == mLaneStandardChannel)
			continue;

		//cout << "Create filtered signal for channel " << i << endl;
		mDataChannels [i]->CreateAndSubstituteFilteredSignalForRawData (window);
		//cout << "Have created filtered signal for channel " << i << endl;
	}

	return 0;
}



int CoreBioComponent :: RestoreRawDataAndDeleteFilteredSignalNonILS () {

	int i;

	for (i=1; i<=mNumberOfChannels; i++) {

		if (i == mLaneStandardChannel)
			continue;

		mDataChannels [i]->RestoreRawDataAndDeleteFilteredSignal ();
	}

	return 0;
}



int CoreBioComponent :: AddNoticeToList (Notice* newNotice) {

	if (NewNoticeList.Entries () == 0) {

		mHighestSeverityLevel = newNotice->GetSeverityLevel ();
		mHighestMessageLevel = newNotice->GetMessageLevel ();
		NewNoticeList.Append (newNotice);
	}

	else {

		int temp = newNotice->GetSeverityLevel ();

		if (mHighestSeverityLevel >= temp) {

			mHighestSeverityLevel = temp;
			NewNoticeList.Prepend (newNotice);
		}

		else {

			RGDListIterator it (NewNoticeList);
			Notice* nextNotice;
			bool noticeInserted = false;

			while (nextNotice = (Notice*) it ()) {

				if (temp < nextNotice->GetSeverityLevel ()) {

					--it;
					it.InsertAfterCurrentItem (newNotice);
					noticeInserted = true;
					break;
				}
			}

			if (!noticeInserted)
				NewNoticeList.Append (newNotice);
		}

		temp = newNotice->GetMessageLevel ();

		if (mHighestMessageLevel > temp)
			mHighestMessageLevel = temp;
	}

	return NewNoticeList.Entries ();
}



Boolean CoreBioComponent :: IsNoticeInList (const Notice* target) {

	if (NewNoticeList.Find (target))
		return TRUE;

	return FALSE;
}


Notice* CoreBioComponent :: GetNotice (const Notice* target) {

	return (Notice*) NewNoticeList.Find (target);
}



Boolean CoreBioComponent :: ReportNoticeObjects (RGTextOutput& text, const RGString& indent, const RGString& delim, Boolean reportLink) {

	if (NewNoticeList.Entries () > 0) {

		RGDListIterator it (NewNoticeList);
		Notice* nextNotice;
		text.SetOutputLevel (mHighestMessageLevel);
		RGString indent2 = indent + "\t";

		if (!text.TestCurrentLevel ()) {

			text.ResetOutputLevel ();
			return FALSE;
		}

		Endl endLine;
		text << endLine;

		if (reportLink)
			text << mTableLink;

		text << indent << GetSampleName () << " Notices:  " << endLine;
		text.ResetOutputLevel ();

		while (nextNotice = (Notice*) it ())
			nextNotice->Report (text, indent2, delim);

		if (reportLink) {

			text.SetOutputLevel (mHighestMessageLevel);
			text << mTableLink;
			text.ResetOutputLevel ();
		}

		text.Write (1, "\n");
	}

	else
		return FALSE;

	return TRUE;
}


Boolean CoreBioComponent :: ReportXMLNoticeObjects (RGTextOutput& text, RGTextOutput& tempText, const RGString& delim) {

	if (NewNoticeList.Entries () > 0) {

		RGDListIterator it (NewNoticeList);
		Notice* nextNotice;
		text.SetOutputLevel (mHighestMessageLevel);
		tempText.SetOutputLevel (1);
		int msgNum;
		int triggerLevel = Notice::GetMessageTrigger ();

		if (!text.TestCurrentLevel ()) {

			text.ResetOutputLevel ();
			tempText.ResetOutputLevel ();
			return FALSE;
		}

		while (nextNotice = (Notice*) it ()) {

			if (nextNotice->GetMessageLevel () <= triggerLevel) {

				msgNum = Notice::GetNextMessageNumber ();
				nextNotice->SetMessageNumber (msgNum);
				text << "\t\t\t\t<MessageNumber>" << msgNum << "</MessageNumber>\n";
				tempText << "\t\t<Message>\n";
				tempText << "\t\t\t<MessageNumber>" << msgNum << "</MessageNumber>\n";
				tempText << "\t\t\t<Index>" << nextNotice->GetID () << "</Index>\n";
				tempText << "\t\t\t<Text>" << (nextNotice->AssembleString (delim)).GetData () << "</Text>\n";
				tempText << "\t\t</Message>\n";
			}
		}

		text.ResetOutputLevel ();
		tempText.ResetOutputLevel ();
	}

	else
		return FALSE;

	return TRUE;
}


void CoreBioComponent :: ClearNoticeObjects () {

	NewNoticeList.ClearAndDelete ();
}



int CoreBioComponent :: NumberOfNoticeObjects () {

	return NewNoticeList.Entries ();
}


RGString CoreBioComponent :: IsNoticeInAnyLocusList (const Notice* target) {

	RGString info;
	RGString totalInfo;

	for (int i=1; i<=mNumberOfChannels; i++) {

		if (i != mLaneStandardChannel) {

			info = mDataChannels [i]->IsNoticeInAnyLocusList (target);

			if (info.Length () > 0) {

				if (totalInfo.Length () > 0)
					totalInfo << ", " << info;

				else
					totalInfo << "Locus " << info;
			}
		}
	}
	
	return totalInfo;
}


RGString CoreBioComponent :: IsNoticeInAnyChannelList (const Notice* target) {

	RGString totalInfo;

	for (int i=1; i<=mNumberOfChannels; i++) {

		if (i != mLaneStandardChannel) {

			if (mDataChannels [i]->IsNoticeInList (target)) {

				if (totalInfo.Length () > 0)
					totalInfo << ", " << i;

				else
					totalInfo << "Channel " << i;
			}
		}
	}
	
	return totalInfo;
}



int CoreBioComponent :: GetLocusAndChannelHighestMessageLevel () {

	int highest = mLSData->GetHighestMessageLevel ();

	if (highest < 0)
		highest = mHighestMessageLevel;

	else if ((mHighestMessageLevel > 0) && (mHighestMessageLevel < highest))
		highest = mHighestMessageLevel;

	int channelHighest;

	for (int i=1; i<=mNumberOfChannels; i++) {

		if (i != mLaneStandardChannel) {

			channelHighest = mDataChannels [i]->GetHighestMessageLevelFromLoci ();

			if (channelHighest < 0)
				continue;

			if (highest < 0)
				highest = channelHighest;

			else if (channelHighest < highest)
				highest = channelHighest;
		}
	}
	
	return highest;
}


void CoreBioComponent :: UpdateNoticesFromLoci () {

	int highest = GetLocusAndChannelHighestMessageLevel ();
	Notice* newNotice;

	if ((highest > 0) && (highest <= Notice::GetMessageTrigger ())) {

		newNotice = new SampleReqReview;

		if (IsNoticeInList (newNotice))
			delete newNotice;

		else
			AddNoticeToList (newNotice);
	}
}


Boolean CoreBioComponent :: ReportAllNoticeObjects (RGTextOutput& text, const RGString& indent, const RGString& delim, Boolean reportLinks) {

	int severity;	
	Boolean reportedNotices = ReportNoticeObjects (text, indent, delim, reportLinks);

	if (!reportedNotices) {

		severity = GetLocusAndChannelHighestMessageLevel ();

		if ((severity > 0) && (severity <= Notice::GetMessageTrigger())) {

			text.SetOutputLevel (severity);
			Endl endLine;
			text << indent << GetSampleName () << " Notices:  " << endLine;
			text.ResetOutputLevel ();
		}
	}

	RGString indent2 = indent + "\t";
	mLSData->ReportNoticeObjects (text, indent2, delim, reportLinks);

	for (int i=1; i<=mNumberOfChannels; i++) {

		if (i != mLaneStandardChannel)
			mDataChannels[i]->ReportAllNoticeObjects (text, indent2, delim, reportLinks);
	}

	return TRUE;
}


Boolean CoreBioComponent :: PrepareLociForOutput () {

	RGString locusName;
	Locus* nextLocus;
	Locus* myLocus;
	mMarkerSet->ResetLocusList ();

	while (nextLocus = mMarkerSet->GetNextLocus ()) {

		myLocus = FindLocus (nextLocus->GetLocusChannel (), nextLocus->GetLocusName ());

		if (myLocus == NULL)
			continue;

		myLocus->BuildSampleString (",");
	}

	return TRUE;
}


int CoreBioComponent :: MakePreliminaryCalls (GenotypesForAMarkerSet* pGenotypes) {

	for (int i=1; i<=mNumberOfChannels; i++) {

		if (i != mLaneStandardChannel)
			mDataChannels [i]->MakePreliminaryCalls (mIsNegativeControl, mIsPositiveControl, pGenotypes);
	}

	return 0;
}


void CoreBioComponent :: ReportSampleData (RGTextOutput& ExcelText) {

	RGString indent = "     ";
	RGString indent2 = indent + indent;

	for (int i=1; i<=mNumberOfChannels; i++) {

		if (i != mLaneStandardChannel) {

			mDataChannels [i]->ReportSampleData (ExcelText, indent);
			mDataChannels [i]->ReportArtifacts (ExcelText, indent2);
		}
	}
}


void CoreBioComponent :: ReportSampleTableRow (RGTextOutput& text) {

	RGString locusName;
	Locus* nextLocus;
	Locus* myLocus;
	mMarkerSet->ResetLocusList ();
	bool reportedChannel = false;
	int severity;

	//  First output sample name, then ILS

	text.SetOutputLevel (1);
	text << GetSampleName () << "\t";

	if ((mLSData->GetHighestMessageLevel () > 0) && 
		(mLSData->GetHighestMessageLevel () <= Notice::GetMessageTrigger()))
		text << "XX";

	else
		text << "OK";

	for (int i=1; i<=mNumberOfChannels; i++) {

		if (i != mLaneStandardChannel) {

			severity = mDataChannels [i]->GetHighestMessageLevel ();

			if ((severity > 0) && (severity <= Notice::GetMessageTrigger ())) {

				if (reportedChannel)
					text << ", " << i;

				else {

					reportedChannel = true;
					text << "\t" << i;
				}
			}
		}
	}

	if (!reportedChannel)
		text << "\t";

	while (nextLocus = mMarkerSet->GetNextLocus ()) {

		text << "\t";
		myLocus = FindLocus (nextLocus->GetLocusChannel (), nextLocus->GetLocusName ());

		if (myLocus == NULL)
			continue;

		text << myLocus->GetSampleString ();
	}

	text << "\t" << mPositiveControlName;
	text << "\n";
	text.ResetOutputLevel ();
}


void CoreBioComponent :: ReportSampleTableRowWithLinks (RGTextOutput& text) {

	RGString locusName;
	Locus* nextLocus;
	Locus* myLocus;
	mMarkerSet->ResetLocusList ();
	int link;
	RGString LinkString;
	int trigger = Notice::GetMessageTrigger ();
	int highest;
	bool reportedChannel = false;
	int severity;

	//  First output sample name, then ILS

	text.SetOutputLevel (1);

	if ((mHighestSeverityLevel > 0) && (mHighestSeverityLevel <= trigger)) {

		link = Notice::GetNextLinkNumber ();
		SetTableLink (link);
		text << mTableLink << GetSampleName () << "\t";
	}

	else
		text << GetSampleName () << "\t";

	highest = mLSData->GetHighestMessageLevel ();

	if ((highest > 0) && (highest <= trigger)) {

			link = Notice::GetNextLinkNumber ();
			mLSData->SetTableLink (link);
			text << mLSData->GetTableLink () << "XX";
	}

	else
		text << "OK";

	for (int i=1; i<=mNumberOfChannels; i++) {

		if (i != mLaneStandardChannel) {

			severity = mDataChannels [i]->GetHighestMessageLevel ();

			if ((severity > 0) && (severity <= Notice::GetMessageTrigger ())) {

				if (reportedChannel) {

					text << ", " << i;
					mDataChannels [i]->SetTableLink (link);
				}

				else {

					reportedChannel = true;
					link = Notice::GetNextLinkNumber ();
					mDataChannels [i]->SetTableLink (link);
					text << "\t" << mDataChannels [i]->GetTableLink () << i;
				}
			}
		}
	}

	if (!reportedChannel)
		text << "\t";

	while (nextLocus = mMarkerSet->GetNextLocus ()) {

		text << "\t";
		myLocus = FindLocus (nextLocus->GetLocusChannel (), nextLocus->GetLocusName ());

		if (myLocus == NULL)
			continue;

		highest = myLocus->GetHighestMessageLevel ();

		if ((highest > 0) && (highest <= trigger)) {

			link = Notice::GetNextLinkNumber ();
			myLocus->SetTableLink (link);
			text << myLocus->GetTableLink () << myLocus->GetSampleString ();
		}

		else
			text << myLocus->GetSampleString ();
	}

	text << "\t" << mPositiveControlName;
	text << "\n";
	text.ResetOutputLevel ();
}


void CoreBioComponent :: ReportGridTableRow (RGTextOutput& text) {

	bool reportedChannel = false;
	int highest;
	int trigger = Notice::GetMessageTrigger ();
	int severity;

	//  First output grid name, then ILS

	text.SetOutputLevel (1);
	text << GetSampleName () << "\t";
	highest = mLSData->GetHighestMessageLevel ();

	if ((highest > 0) && (highest <= trigger))
		text << "XX";

	else
		text << "OK";
  int i;
	for (i=1; i<=mNumberOfChannels; i++) {

		if (i != mLaneStandardChannel) {

			severity = mDataChannels [i]->GetHighestMessageLevel ();

			if ((severity > 0) && (severity <= trigger)) {

				if (reportedChannel)
					text << ", " << i;

				else {

					reportedChannel = true;
					text << "\t" << i;
				}
			}
		}
	}

	if (!reportedChannel)
		text << "\t";

	for (i=1; i<=mNumberOfChannels; i++) {

		if (i != mLaneStandardChannel)
			mDataChannels [i]->ReportGridLocusRow (text);
	}

	text << "\t" << mPositiveControlName;
	text << "\n";
	text.ResetOutputLevel ();
}



void CoreBioComponent :: ReportGridTableRowWithLinks (RGTextOutput& text) {

	int link;
	RGString LinkString;
	bool reportedChannel = false;
	int trigger = Notice::GetMessageTrigger ();
	int highest;

	//  First output sample name, then ILS

	text.SetOutputLevel (1);

	if ((mHighestMessageLevel > 0) && (mHighestMessageLevel <= trigger)) {

		link = Notice::GetNextLinkNumber ();
		SetTableLink (link);
		text << mTableLink << GetSampleName () << "\t";
	}

	else
		text << GetSampleName () << "\t";

	highest = mLSData->GetHighestMessageLevel ();

	if ((highest > 0) && (highest <= trigger)) {

		link = Notice::GetNextLinkNumber ();
		mLSData->SetTableLink (link);
		text << mLSData->GetTableLink () << "XX";
	}

	else
		text << "OK";
  int i;
	for (i=1; i<=mNumberOfChannels; i++) {

		if (i != mLaneStandardChannel) {

			highest = mDataChannels [i]->GetHighestMessageLevel ();

			if ((highest > 0) && (highest <= trigger)) {

				if (reportedChannel) {

					text << ", " << i;
					mDataChannels [i]->SetTableLink (link);
				}

				else {

					reportedChannel = true;
					link = Notice::GetNextLinkNumber ();
					mDataChannels [i]->SetTableLink (link);
					text << "\t" << mDataChannels [i]->GetTableLink () << i;
				}
			}
		}
	}

	if (!reportedChannel)
		text << "\t";

	for (i=1; i<=mNumberOfChannels; i++) {

		if (i != mLaneStandardChannel)
			mDataChannels [i]->ReportGridLocusRowWithLinks (text);
	}

	text << "\t" << mPositiveControlName;
	text << "\n";
	text.ResetOutputLevel ();
}


void CoreBioComponent :: ReportXMLGridTableRowWithLinks (RGTextOutput& text, RGTextOutput& tempText) {

	text << CLevel (1) << "\t\t<Sample>\n";
	RGString SimpleFileName (mName);
	size_t startPos = 0;
	size_t endPos;
	size_t length = SimpleFileName.Length ();
	RGString pResult;
	
	if (SimpleFileName.FindLastSubstringCaseIndependent (DirectoryManager::GetDataFileType (), startPos, endPos)) {

		if (endPos == length - 1)
			SimpleFileName.ExtractAndRemoveLastCharacters (4);
	}

	SimpleFileName.FindAndReplaceAllSubstrings ("\\", "/");
	startPos = endPos = 0;

	if (SimpleFileName.FindLastSubstring ("/", startPos, endPos)) {

		SimpleFileName.ExtractAndRemoveSubstring (0, startPos);
	}

	text << "\t\t\t<Name>" << xmlwriter::EscAscii (SimpleFileName, &pResult) << "</Name>\n";
	text << "\t\t\t<Type>Ladder</Type>\n" << PLevel ();

	int trigger = Notice::GetMessageTrigger ();
	int channelHighestLevel;
	bool channelAlerts = false;

	if ((mHighestMessageLevel > 0) && (mHighestMessageLevel <= trigger)) {

		text << CLevel (1) << "\t\t\t<SampleAlerts>\n" << PLevel ();

		// get message numbers and report
		ReportXMLNoticeObjects (text, tempText, " ");

		text << CLevel (1) << "\t\t\t</SampleAlerts>\n" << PLevel ();
	}

	mDataChannels [mLaneStandardChannel]->ReportXMLILSNoticeObjects (text, tempText, " ");

	int i;

	for (i=1; i<=mNumberOfChannels; i++) {

		if (i == mLaneStandardChannel)
			continue;

		channelHighestLevel = mDataChannels [i]->GetHighestMessageLevel ();

		if ((channelHighestLevel > 0) && (channelHighestLevel <= trigger)) {

			channelAlerts = true;
			break;
		}
	}

	if (channelAlerts) {

		text << CLevel (1) << "\t\t\t<ChannelAlerts>\n" << PLevel ();

		for (i=1; i<=mNumberOfChannels; i++) {

			if (i == mLaneStandardChannel)
				continue;

			mDataChannels [i]->ReportXMLNoticeObjects (text, tempText, " ");
		}

		text << CLevel (1) << "\t\t\t</ChannelAlerts>\n" << PLevel ();
	}

	mMarkerSet->ResetLocusList ();
	Locus* nextLocus;
	int locusHighestLevel;

	while (nextLocus = mMarkerSet->GetNextLocus ()) {

		locusHighestLevel = nextLocus->GetHighestMessageLevel ();

		if ((locusHighestLevel > 0) && (locusHighestLevel <= trigger))
			nextLocus->ReportXMLGridTableRowWithLinks (text, tempText, " ");
	}

	text << CLevel (1) << "\t\t</Sample>\n" << PLevel ();
}



void CoreBioComponent :: ReportXMLSampleTableRowWithLinks (RGTextOutput& text, RGTextOutput& tempText) {

	RGString type;

	if (mIsNegativeControl)
		type = "-Control";

	else if (mIsPositiveControl)
		type = "+Control";

	else
		type = "Sample";

	RGString pResult;

	RGString SimpleFileName (mName);
	size_t startPos = 0;
	size_t endPos;
	size_t length = SimpleFileName.Length ();
	
	if (SimpleFileName.FindLastSubstringCaseIndependent (DirectoryManager::GetDataFileType (), startPos, endPos)) {

		if (endPos == length - 1)
			SimpleFileName.ExtractAndRemoveLastCharacters (4);
	}

	SimpleFileName.FindAndReplaceAllSubstrings ("\\", "/");
	startPos = endPos = 0;

	if (SimpleFileName.FindLastSubstring ("/", startPos, endPos)) {

		SimpleFileName.ExtractAndRemoveSubstring (0, startPos);
	}
	
	text << CLevel (1) << "\t\t<Sample>\n";
	text << "\t\t\t<Name>" << xmlwriter::EscAscii (SimpleFileName, &pResult) << "</Name>\n";
	text << "\t\t\t<RunStart>" << mRunStart.GetData () << "</RunStart>\n";
	text << "\t\t\t<Type>" << type.GetData () << "</Type>\n" << PLevel ();

	int trigger = Notice::GetMessageTrigger ();
	int channelHighestLevel;
	bool channelAlerts = false;

	if ((mHighestMessageLevel > 0) && (mHighestMessageLevel <= trigger)) {

		text << CLevel (1) << "\t\t\t<SampleAlerts>\n" << PLevel ();

		// get message numbers and report
		ReportXMLNoticeObjects (text, tempText, " ");

		text << CLevel (1) << "\t\t\t</SampleAlerts>\n" << PLevel ();
	}

	mDataChannels [mLaneStandardChannel]->ReportXMLILSNoticeObjects (text, tempText, " ");
  int i;
	for (i=1; i<=mNumberOfChannels; i++) {

		if (i == mLaneStandardChannel)
			continue;

		channelHighestLevel = mDataChannels [i]->GetHighestMessageLevel ();

		if ((channelHighestLevel > 0) && (channelHighestLevel <= trigger)) {

			channelAlerts = true;
			break;
		}
	}

	if (channelAlerts) {

		text << CLevel (1) << "\t\t\t<ChannelAlerts>\n" << PLevel ();

		for (i=1; i<=mNumberOfChannels; i++) {

			if (i == mLaneStandardChannel)
				continue;

			mDataChannels [i]->ReportXMLNoticeObjects (text, tempText, " ");
		}

		text << CLevel (1) << "\t\t\t</ChannelAlerts>\n" << PLevel ();
	}

	mMarkerSet->ResetLocusList ();
	Locus* nextLocus;

	while (nextLocus = mMarkerSet->GetNextLocus ()) {

		nextLocus->ReportXMLSampleTableRowWithLinks (text, tempText, " ");
	}

	if (mIsPositiveControl)
		text << CLevel (1) << "\t\t\t<PositiveControl>" << mPositiveControlName << "</PositiveControl>\n";

	text << CLevel (1) << "\t\t</Sample>\n" << PLevel ();
}



CoreBioComponent* CoreBioComponent :: GetBestGridBasedOnTimeForAnalysis (RGDList& gridList) {

	RGDListIterator it (gridList);
	CoreBioComponent* nextGrid;
	CoreBioComponent* minGrid;

	minGrid = (CoreBioComponent*) it ();
	int minTimeSeparation = TimeSeparation (minGrid);
	int currentTimeSeparation;

	while (nextGrid = (CoreBioComponent*) it()) {

		currentTimeSeparation = TimeSeparation (nextGrid);

		if (currentTimeSeparation < minTimeSeparation) {

			minTimeSeparation = currentTimeSeparation;
			minGrid = nextGrid;
		}
	}

	return minGrid;
}


CoreBioComponent* CoreBioComponent :: GetBestGridBasedOnMax2DerivForAnalysis (RGDList& gridList, CSplineTransform*& timeMap) {

	RGDListIterator it (gridList);
	CoreBioComponent* nextGrid;
	CoreBioComponent* minGrid;
	CSplineTransform* nextTrans;
	double min2Deriv;
	double current2Deriv;
	smLadderFitThreshold ladderFitThreshold;
	smSampleToLadderFitBelowExpectations ladderFitPoor;

	minGrid = NULL;
	timeMap = NULL;
	min2Deriv = DOUBLEMAX;

	while (nextGrid = (CoreBioComponent*) it()) {

		nextTrans = TimeTransform (*this, *nextGrid);	// Could augment calling sequence to use Hermite Cubic Spline transform 04/10/2014

		if (nextTrans == NULL)
			continue;

		current2Deriv = nextTrans->MaxSecondDerivative ();

		if (current2Deriv < min2Deriv) {

			min2Deriv = current2Deriv;
			delete timeMap;
			minGrid = nextGrid;
			timeMap = nextTrans;
		}

		else
			delete nextTrans;
	}

	cout << "Best grid for sample file " << (char*)mName.GetData () << " is ladder " << (char*)minGrid->GetSampleName ().GetData () << " with min 2nd deriv " << (int) ceil (min2Deriv * 1.0e6) << "\n";
	int scaledMin2Deriv = (int)ceil (min2Deriv * 1.0e6);
	int threshold = GetThreshold (ladderFitThreshold);

	if (scaledMin2Deriv >= threshold) {

		SetMessageValue (ladderFitPoor, true);
		AppendDataForSmartMessage (ladderFitPoor, scaledMin2Deriv);
	}

	return minGrid;
}


CoreBioComponent* CoreBioComponent :: GetBestGridBasedOnLeastTransformError (RGDList& gridList, CSplineTransform*& timeMap, const double* characteristicArray) {

	RGDListIterator it (gridList);
	CoreBioComponent* nextGrid;
	CoreBioComponent* minGrid;
	CSplineTransform* nextTrans;
	double maxError;
	double errorBound;
	smLadderFitThreshold ladderFitThreshold;
	smSampleToLadderFitBelowExpectations ladderFitPoor;

	minGrid = NULL;
	timeMap = NULL;
	maxError = DOUBLEMAX;

	while (nextGrid = (CoreBioComponent*) it()) {

		nextTrans = TimeTransform (*this, *nextGrid);	// Could augment calling sequence to use Hermite Cubic Spline transform 04/10/2014

		if (nextTrans == NULL)
			continue;

		errorBound = nextTrans->GetMaximumErrorOfInterpolation (characteristicArray);

		if (errorBound < maxError) {

			maxError = errorBound;
			delete timeMap;
			minGrid = nextGrid;
			timeMap = nextTrans;
		}

		else
			delete nextTrans;
	}

	cout << "Best grid based on transform error for sample file " << (char*)mName.GetData () << " is ladder " << (char*)minGrid->GetSampleName ().GetData () << " with max error " << maxError << " bps\n";
	//int scaledMin2Deriv = (int)ceil (min2Deriv * 1.0e6);
	//int threshold = GetThreshold (ladderFitThreshold);

	//if (scaledMin2Deriv >= threshold) {

	//	SetMessageValue (ladderFitPoor, true);
	//	AppendDataForSmartMessage (ladderFitPoor, scaledMin2Deriv);
	//}

	return minGrid;
}


CoreBioComponent* CoreBioComponent :: GetBestGridBasedOnMaxDelta3DerivForAnalysis (RGDList& gridList, CSplineTransform*& timeMap) {

	RGDListIterator it (gridList);
	CoreBioComponent* nextGrid;
	CoreBioComponent* minGrid;
	CSplineTransform* nextTrans;
	double min3Deriv;
	double current3Deriv;

	minGrid = NULL;
	timeMap = NULL;
	min3Deriv = DOUBLEMAX;

	while (nextGrid = (CoreBioComponent*) it()) {

		nextTrans = TimeTransform (*this, *nextGrid);	// Could augment calling sequence to use Hermite Cubic Spline transform 04/10/2014

		if (nextTrans == NULL)
			continue;

		current3Deriv = nextTrans->MaxDeltaThirdDerivative ();

		if (current3Deriv < min3Deriv) {

			min3Deriv = current3Deriv;
			delete timeMap;
			minGrid = nextGrid;
			timeMap = nextTrans;
		}

		else
			delete nextTrans;
	}

	return minGrid;
}


int CoreBioComponent :: Initialize (SampleData& fileData, PopulationCollection* collection, const RGString& markerSetName, Boolean isGrid) {

	mTime = fileData.GetCollectionStartTime ();
	mDate = fileData.GetCollectionStartDate ();
	mName = fileData.GetName ();
	mRunStart = mDate.GetOARString () + mTime.GetOARString ();
//	mNumberOfChannels = fileData.GetNumberOfDataChannels ();
	mMarkerSet = collection->GetNamedPopulationMarkerSet (markerSetName);
	Progress = 0;
	Notice* newNotice;

	if (mMarkerSet == NULL) {

		ErrorString = "*******COULD NOT FIND MARKER SET NAMED ";
		ErrorString << markerSetName << " IN POPULATION COLLECTION********\n";
		newNotice = new NoNamedMarkerSet;
		newNotice->AddDataItem (markerSetName);
		AddNoticeToList (newNotice);
		return -1;
	}

	mLaneStandard = mMarkerSet->GetLaneStandard ();
	mNumberOfChannels = mMarkerSet->GetNumberOfChannels ();

	if (mLaneStandard == NULL) {

		ErrorString = "Could not find named internal lane standard associated with marker set named ";
		ErrorString << markerSetName << "\n";
		newNotice = new NoNamedLaneStandard;
		AddNoticeToList (newNotice);
		return -1;
	}

	mDataChannels = new ChannelData* [mNumberOfChannels + 1];
	int i;
	const int* fsaChannelMap = mMarkerSet->GetChannelMap ();

	for (i=0; i<=mNumberOfChannels; i++)
		mDataChannels [i] = NULL;

	mLaneStandardChannel = mMarkerSet->GetLaneStandardChannel ();

	for (i=1; i<=mNumberOfChannels; i++) {

		if (i == mLaneStandardChannel)
			mDataChannels [i] = GetNewLaneStandardChannel (i, mLaneStandard);

		else {

			if (isGrid)
				mDataChannels [i] = GetNewGridDataChannel (i, mLaneStandard);

			else
				mDataChannels [i] = GetNewDataChannel (i, mLaneStandard);
		}

		mDataChannels [i]->SetFsaChannel (fsaChannelMap [i]);
		mChannelList.Append (mDataChannels [i]);
	}

	mLSData = mDataChannels [mLaneStandardChannel];
	mMarkerSet->ResetLocusList ();
	Locus* nextLocus;

	while (nextLocus = mMarkerSet->GetNextLocus ())
		mDataChannels [nextLocus->GetLocusChannel ()]->AddLocus (nextLocus);

	Progress = 1;
	return 0;
}


int CoreBioComponent :: SetAllData (SampleData& fileData, TestCharacteristic* testControlPeak, TestCharacteristic* testSamplePeak) {

	int status = 0;
	ErrorString = "";

	for (int i=1; i<=mNumberOfChannels; i++) {

		if (mDataChannels [i]->SetData (fileData, testControlPeak, testSamplePeak) < 0) {

			ErrorString << mDataChannels [i]->GetError ();
			status = -1;
		}
	}

	if (status == 0)
		Progress = 2;

	return status;
}


int CoreBioComponent :: SetAllRawData (SampleData& fileData, TestCharacteristic* testControlPeak, TestCharacteristic* testSamplePeak) {

	int status = 0;
	ErrorString = "";

	for (int i=1; i<=mNumberOfChannels; i++) {

		if (mDataChannels [i]->SetRawData (fileData, testControlPeak, testSamplePeak) < 0) {

			ErrorString << mDataChannels [i]->GetError ();
			status = -1;
		}
	}

	if (status == 0)
		Progress = 2;

	return status;
}


int CoreBioComponent :: FindAndRemoveFixedOffsets () {

	int status = 0;
	ErrorString = "";

	for (int i=1; i<=mNumberOfChannels; i++) {

		if (mDataChannels [i]->FindAndRemoveFixedOffset () < 0) {

			ErrorString << "Channel " << i << " could not find offset accurately";
			status = -1;
		}
	}

	return status;
}


int CoreBioComponent :: SetLaneStandardData (SampleData& fileData, TestCharacteristic* testControlPeak, TestCharacteristic* testSamplePeak) {

	int status = mDataChannels [mLaneStandardChannel]->SetRawData (fileData, testControlPeak, testSamplePeak);

	if (status < 0)
		ErrorString << mDataChannels [mLaneStandardChannel]->GetError ();

	return status;
}


int CoreBioComponent :: FitAllCharacteristics (RGTextOutput& text, RGTextOutput& ExcelText, OsirisMsg& msg, Boolean print) {

	int status = 0;

	for (int i=1; i<=mNumberOfChannels; i++) {

		if (mDataChannels [i]->FitAllCharacteristics (text, ExcelText, msg, print) < 0) {

			ErrorString << mDataChannels [i]->GetError ();
			status = -i;
		}
	}

	return status;
}


int CoreBioComponent :: FitNonLaneStandardCharacteristics (RGTextOutput& text, RGTextOutput& ExcelText, OsirisMsg& msg, Boolean print) {

	int status = 0;

	for (int i=1; i<=mNumberOfChannels; i++) {

		if (i != mLaneStandardChannel) {

			if (mDataChannels [i]->FitAllCharacteristics (text, ExcelText, msg, print) < 0) {

				ErrorString << mDataChannels [i]->GetError ();
				status = -i;
			}
		}
	}

	return status;
}


int CoreBioComponent :: FitLaneStandardCharacteristics (RGTextOutput& text, RGTextOutput& ExcelText, OsirisMsg& msg, Boolean print) {

	int status = mDataChannels [mLaneStandardChannel]->FitAllCharacteristics (text, ExcelText, msg, print);

	if (status < 0)
		ErrorString << mDataChannels [mLaneStandardChannel]->GetError ();

	return status;
}


int CoreBioComponent :: AnalyzeLaneStandardChannel (RGTextOutput& text, RGTextOutput& ExcelText, OsirisMsg& msg, Boolean print) {

	int status;
	status =  mDataChannels [mLaneStandardChannel]->HierarchicalLaneStandardChannelAnalysis (text, ExcelText, msg, print);

	if (status < 0)
		ErrorString << mDataChannels [mLaneStandardChannel]->GetError ();

	return status;
}


int CoreBioComponent :: AssignCharacteristicsToLoci () {

	int status = 0;
	
	for (int i=1; i<=mNumberOfChannels; i++) {

		if (i != mLaneStandardChannel) {

			if (!mDataChannels [i]->AssignCharacteristicsToLoci (mLSData)) {

				ErrorString << mDataChannels [i]->GetError ();
				status = -1;
			}
		}
	}

	return status;
}


int CoreBioComponent :: AnalyzeGridLoci (RGTextOutput& text, RGTextOutput& ExcelText, OsirisMsg& msg, Boolean print) {

	return -1;
}


int CoreBioComponent :: AnalyzeSampleLoci (RGTextOutput& text, RGTextOutput& ExcelText, OsirisMsg& msg, Boolean print) {

	return -1;
}


int CoreBioComponent :: AnalyzeCrossChannel () {

	return 0;
}


int CoreBioComponent :: OrganizeNoticeObjects () {

	if (Progress < 4)
		return 0;

	for (int j=1; j<=mNumberOfChannels; j++)
		mDataChannels [j]->TestArtifactListForNoticesWithinLaneStandard (mLSData, mAssociatedGrid);

	return 0;
}


int CoreBioComponent :: AnalyzeGrid (RGTextOutput& text, RGTextOutput& ExcelText, OsirisMsg& msg, Boolean print) {

	return -1;
}


int CoreBioComponent :: AnalyzeGrid (SampleData& fileData, GridDataStruct* gridData) {

	Endl endLine;
	RGString Notice;
	int status = Initialize (fileData, gridData->mCollection, gridData->mMarkerSetName, TRUE);

	if (status < 0) {

		Notice << "BioComponent could not initialize:";
		cout << Notice << endl;
		gridData->mExcelText << CLevel (1) << Notice << "\n" << ErrorString << "Skipping...\n" << PLevel ();
		gridData->mText << Notice << "\n" << ErrorString << "Skipping\n";
		return -1;
	}

	if (CoreBioComponent::UseRawData)
		status = SetAllRawData (fileData, gridData->mTestControlPeak, gridData->mTestControlPeak);
	
	else		
		status = SetAllData (fileData, gridData->mTestControlPeak, gridData->mTestControlPeak);

	if (status < 0) {

		Notice << "BioComponent could not set data:";
		cout << Notice << endl;
		gridData->mExcelText << CLevel (1) << Notice << "\n" << ErrorString << "Skipping...\n" << PLevel ();
		gridData->mText << Notice << "\n" << ErrorString << "Skipping...\n";
		return -2;
	}

	if (CoreBioComponent::UseRawData)
		FindAndRemoveFixedOffsets ();

	status = AnalyzeGrid (gridData->mText, gridData->mExcelText, gridData->mMsg);

	if (status < 0) {

		Notice << "BioComponent could not analyze grid.  Skipping...";
		cout << Notice << endl;
		Notice << "\n";
		gridData->mExcelText.Write (1, Notice);
		gridData->mText << Notice;
		return -3;
	}

	return 0;
}


int CoreBioComponent :: PrepareSampleForAnalysis (SampleData& fileData, SampleDataStruct* sampleData) {

	Endl endLine;

	RGString notice;
	notice << "Analyzing sample named " << GetSampleName () << "\n";
	sampleData->mExcelText.Write (1, notice);
	sampleData->mText << notice;
	notice = "";
	Notice* newNotice;
//	BlobFound newNotice;
	Progress = 0;

	int status = Initialize (fileData, sampleData->mCollection, sampleData->mMarkerSetName, FALSE);

	if (status < 0) {

		notice << "BIOCOMPONENT COULD NOT INITIALIZE:";
		cout << notice << endl;
		sampleData->mExcelText.Write (1, notice);
		sampleData->mExcelText << CLevel (1) << notice << "\n" << ErrorString << "Skipping...\n" << PLevel ();
		sampleData->mText << notice << "\n" << ErrorString << "Skipping...\n";
		return -1;
	}

	Progress = 1;

	if (CoreBioComponent::UseRawData)
		status = SetAllRawData (fileData, sampleData->mTestControlPeak, sampleData->mTestSamplePeak);
	
	else		
		status = SetAllData (fileData, sampleData->mTestControlPeak, sampleData->mTestSamplePeak);

	if (status < 0) {

		notice << "BIOCOMPONENT COULD NOT SET DATA:";
		cout << notice << endl;
		sampleData->mExcelText << CLevel (1) << notice << "\n" << ErrorString << "Skipping...\n" << PLevel ();
		sampleData->mText << notice << "\n" << ErrorString << "Skipping...\n";
		return -2;
	}

	Progress = 2;

	if (CoreBioComponent::UseRawData) {

		status = FindAndRemoveFixedOffsets ();

		if (status < 0) {

			notice << "BIOCOMPONENT COULD NOT COMPUTE OFFSETS ACCURATELY.  Skipping...";
			cout << notice << endl;
			sampleData->mExcelText.Write (1, notice);
			sampleData->mText << notice << "\n" << ErrorString << " Skipping...\n";
			return -5;
		}
	}

//	status = FitAllCharacteristics (sampleData->mText, sampleData->mExcelText, sampleData->mMsg, sampleData->mPrint);
	status = FitAllCharacteristics (sampleData->mText, sampleData->mExcelText, sampleData->mMsg, FALSE);

	if (status < 0) {

		notice << "BIOCOMPONENT COULD NOT FIT ALL CHARACTERISTICS.  Skipping...";
		cout << notice << endl;
		sampleData->mExcelText.Write (1, notice);
		sampleData->mText << notice << "\n" << ErrorString << "Skipping...\n";
		return -3;
	}


	Progress = 3;

	AnalyzeCrossChannel ();
	status = AnalyzeLaneStandardChannel (sampleData->mText, sampleData->mExcelText, sampleData->mMsg, sampleData->mPrint);

	if (status < 0) {

		notice << "BIOCOMPONENT COULD NOT ANALYZE INTERNAL LANE STANDARD.  Skipping...";
		cout << notice << endl;
		sampleData->mExcelText << CLevel (1) << notice << "\n" << ErrorString << "Skipping...\n" << PLevel ();
		sampleData->mText << notice << "\n" << ErrorString << "Skipping...\n";
		newNotice = new ILSReqReview;
		AddNoticeToList (newNotice);

/*		for (int i=1; i<=mNumberOfChannels; i++) {

//			if (i != mLaneStandardChannel) {

				mDataChannels [i]->TestArtifactListForNotices (CoreBioComponent::testChannelArtifactNoticeList);
//			}

//			MergeListAIntoListB (CoreBioComponent::testChannelArtifactNoticeList, NewNoticeList);
		}*/

		return -4;
	}

	status = 0;

	for (int j=1; j<=mNumberOfChannels; j++) {

//		if (j != mLaneStandardChannel) {

			if (mDataChannels [j]->SetAllApproximateIDs (mLSData) < 0)
				status = -1;
//		}
	}

	if (status < 0) {

		notice << "BIOCOMPONENT COULD NOT UTILIZE INTERNAL LANE STANDARD.  Skipping...";
		cout << notice << endl;
		newNotice = new ILSReqReview;
		notice = "Could not create ILS time to base pairs transform";
		newNotice->AddDataItem (notice);
		AddNoticeToList (newNotice);
		return -5;
	}

	Progress = 4;

/*	for (int j=1; j<=mNumberOfChannels; j++) {

//		if (j != mLaneStandardChannel) {

			mDataChannels [j]->TestArtifactListForNoticesWithinLaneStandard (CoreBioComponent::testChannelArtifactNoticeList, mLSData);
//		}
	}*/

	return 0;
}


int CoreBioComponent :: RemoveAllSignalsOutsideLaneStandard () {

	for (int i=1; i<=mNumberOfChannels; i++) {

		if (i != mLaneStandardChannel)
			mDataChannels [i]->RemoveSignalsOutsideLaneStandard (mLSData);
	}

	return 0;
}


int CoreBioComponent :: AssignSampleCharacteristicsToLoci (CoreBioComponent* grid, CoordinateTransform* timeMap) {

	for (int i=1; i<=mNumberOfChannels; i++) {

		if (i != mLaneStandardChannel)
			mDataChannels [i]->AssignSampleCharacteristicsToLoci (grid, timeMap);
	}

	return 0;
}


int CoreBioComponent :: PreliminarySampleAnalysis (RGDList& gridList, SampleDataStruct* sampleData) {

//	CoreBioComponent* grid = GetBestGridBasedOnTimeForAnalysis (gridList);

	CSplineTransform* timeMap;
//	CoreBioComponent* grid = GetBestGridBasedOnMaxDelta3DerivForAnalysis (gridList, timeMap);
	CoreBioComponent* grid = GetBestGridBasedOnMax2DerivForAnalysis (gridList, timeMap);

	if (grid == NULL)
		return -1;

//	CSplineTransform* timeMap = TimeTransform (*this, *grid);
	CSplineTransform* InverseTimeMap = TimeTransform (*grid, *this);	// Could augment calling sequence to use Hermite Cubic Spline transform 04/10/2014

	if (InverseTimeMap != NULL) {

		mAssociatedGrid = grid->CreateNewTransformedBioComponent (*grid, InverseTimeMap);
		delete InverseTimeMap;
	}

	Endl endLine;
	RGString Notice;
	Notice << "ANALYSIS WILL USE GRID NAMED " << grid->GetSampleName () << "\n";
	sampleData->mExcelText.Write (1, Notice);

	RemoveAllSignalsOutsideLaneStandard ();
	int status = AssignSampleCharacteristicsToLoci (grid, timeMap);

	return status;
}


int CoreBioComponent :: ResolveAmbiguousInterlocusSignals () {

	return -1;
}


int CoreBioComponent :: SampleQualityTest (GenotypesForAMarkerSet* genotypes) {

	return -1;
}


int CoreBioComponent :: TestPositiveControl (GenotypesForAMarkerSet* genotypes) {

	IndividualGenotype* genotype;
	Notice* newNotice;
	int returnValue = 0;

	if (!mIsPositiveControl)
		return 0;

	genotype = genotypes->FindGenotypeForFileName (mName);

	if (genotype == NULL) {

		ParameterServer* pServer = new ParameterServer;
		mPositiveControlName = pServer->GetStandardPositiveControlName ();
		genotype = genotypes->FindGenotypeForFileName (mPositiveControlName);
		delete pServer;

		if (genotype == NULL) {
		
			newNotice = new NamedPositiveControlNotFound;
			AddNoticeToList (newNotice);
			return -1;
		}
	}

	else
		mPositiveControlName = genotype->GetName ();

	for (int i=1; i<=mNumberOfChannels; i++) {

		if (i != mLaneStandardChannel) {

			if (mDataChannels [i]->TestPositiveControl (genotype) < 0)
				returnValue = -1;
		}			
	}

	return returnValue;
}


int CoreBioComponent :: LocatePositiveControlName (GenotypesForAMarkerSet* genotypes) {

	IndividualGenotype* genotype;
	
	if (!mIsPositiveControl)
		return 0;

	genotype = genotypes->FindGenotypeForFileName (mName);

	if (genotype == NULL) {

		ParameterServer* pServer = new ParameterServer;
		mPositiveControlName = pServer->GetStandardPositiveControlName ();
		delete pServer;
	}

	else
		mPositiveControlName = genotype->GetName ();
	
	return 0;
}


int CoreBioComponent :: GridQualityTest () {

	return -1;
}


int CoreBioComponent :: FilterNoticesBelowMinBioID () {

	if (mLSData == NULL)
		return 0;

	if (Progress < 4)
		return 0;

	int minBioID = CoreBioComponent::GetMinBioIDForArtifacts ();

	if (minBioID <= 0)
		return 0;

	for (int i=1; i<=mNumberOfChannels; i++)
		mDataChannels [i]->FilterNoticesBelowMinimumBioID (mLSData, minBioID);

	return 0;
}



int CoreBioComponent :: PrepareForILSAnalysis (SampleData& fileData, SampleDataStruct* sampleData) {

	Endl endLine;

	int status = Initialize (fileData, sampleData->mCollection, sampleData->mMarkerSetName, FALSE);

	if (status < 0) {

		cout << "BioComponent could not initialize.  Skipping..." << endl;
		sampleData->mExcelText << "BioComponent could not initialize.  Skipping..." << endLine;
		sampleData->mText << "BioComponent could not initialize.  Skipping..." << endLine;
		return -1;
	}

	status = SetLaneStandardData (fileData, sampleData->mTestControlPeak, sampleData->mTestSamplePeak);

	if (status < 0) {

		cout << "BioComponent could not set internal lane standard data.  Skipping..." << endl;
		sampleData->mExcelText << "BioComponent could not set internal lane standard data.  Skipping..." << endLine;
		sampleData->mText << "BioComponent could not set internal lane standard data.  Skipping..." << endLine;
		return -2;
	}

	status = FitLaneStandardCharacteristics (sampleData->mText, sampleData->mExcelText, sampleData->mMsg, FALSE);

	if (status < 0) {

		cout << "BioComponent could not fit internal lane standard characteristics.  Skipping..." << endl;
		sampleData->mExcelText << "BioComponent could not fit internal lane standard characteristics.  Skipping..." << endLine;
		sampleData->mText << "BioComponent could not fit internal lane standard characteristics.  Skipping..." << endLine;
		return -3;
	}

	status = AnalyzeLaneStandardChannel (sampleData->mText, sampleData->mExcelText, sampleData->mMsg, FALSE);

	if (status < 0) {

		cout << "BioComponent could not analyze internal lane standard channel.  Skipping..." << endl;
		sampleData->mExcelText << "BioComponent could not analyze internal lane standard channel.  Skipping..." << endLine;
		sampleData->mText << "BioComponent could not analyze internal lane standard channel.  Skipping..." << endLine;
		return -4;
	}

	return 0;
}


bool CoreBioComponent :: ComputeExtendedLocusTimes (CoreBioComponent* grid, CoordinateTransform* inverseTransform) {

	int i;
	bool status = true;

	for (i=1; i<=mNumberOfChannels; i++) {

		if (!mDataChannels [i]->ComputeExtendedLocusTimes (grid, inverseTransform, mAssociatedGrid))
			status = false;
	}

	return status;
}


double CoreBioComponent :: GetWidthAtTime (double t) const {

	if ((mLaneStandardChannel <= mNumberOfChannels) && 
		(mDataChannels != NULL) && (mDataChannels [mLaneStandardChannel] != NULL))
		return mDataChannels [mLaneStandardChannel]->GetWidthAtTime (t);

	return -1.0;
}


double CoreBioComponent :: GetSecondaryContentAtTime (double t) const {

	if ((mLaneStandardChannel <= mNumberOfChannels) && 
		(mDataChannels != NULL) && (mDataChannels [mLaneStandardChannel] != NULL))
		return mDataChannels [mLaneStandardChannel]->GetSecondaryContentAtTime (t);

	return -1.0;
}


int CoreBioComponent :: CompareTo (const RGPersistent* p) const {

	CoreBioComponent* q = (CoreBioComponent*) p;

	if (CoreBioComponent::SearchByName) {

		return mName.CompareTo (&q->mName);
	}

	return mTime.Compare (&q->mTime);
}


unsigned CoreBioComponent :: HashNumber (unsigned long Base) const {

	if (CoreBioComponent::SearchByName) {

		return mName.HashNumber (Base);
	}

	return mTime.HashNumber (Base);
}


Boolean CoreBioComponent :: IsEqualTo (const RGPersistent* p) const {

	CoreBioComponent* q = (CoreBioComponent*) p;

	if (CoreBioComponent::SearchByName) {

		return mName.IsEqualTo (&q->mName);
	}

	return mTime.IsEqualTo (&q->mTime);
}

void CoreBioComponent :: RestoreAll (RGFile& f) {

}


void CoreBioComponent :: RestoreAll (RGVInStream& f) {

}


void CoreBioComponent :: SaveAll (RGFile& f) const {

}


void CoreBioComponent :: SaveAll (RGVOutStream& f) const {

}


bool CoreBioComponent :: SampleIsValid () const {

	int i;

	for (i=1; i<=mNumberOfChannels; i++) {

		if (!mDataChannels [i]->ChannelIsValid ())
			return false;
	}

	return true;
}


void CoreBioComponent :: AppendAllBaseLociToList (RGDList& locusList) {

	int i;
	
	for (i=1; i<=mNumberOfChannels; i++)
		mDataChannels [i]->AppendAllBaseLoci (locusList);
}


void CoreBioComponent :: WriteRawDataAndFitData (RGTextOutput& text, SampleData* data) {

	RGString localName (mName);
	localName.ExtractAndRemoveLastCharacters (4);
	size_t start = 0;
	size_t end;
	RGString local2;

	if (localName.FindLastSubstring ("/", start, end)) {

		size_t L = localName.Length ();

		if (end < L - 1)
			local2 = localName.ExtractLastCharacters (L - end - 1);

		else
			local2 = localName;
	}

	RGString fitName = ":Fit" + local2;

	for (int i=1; i<=mNumberOfChannels; i++)
        mDataChannels [i]->WriteRawDataAndFitCurveToOutputRows (text, "\t", data->GetDyeNameForDataChannel (i), fitName);
}


int CoreBioComponent :: WriteXMLGraphicData (const RGString& graphicDirectory, const RGString& localFileName, SampleData* data, int analysisStage, const RGString& intro) {

	return -1;
}


int CoreBioComponent :: WriteFitDataForChannel (int channelNum, RGTextOutput& text, const RGString& delim, ChannelData* cd) {

	mDataChannels [channelNum]->WriteFitData (text, delim, cd->GetNumberOfSamples (), cd->GetLeftEndpoint (), cd->GetRightEndpoint (), false);
	return 0;
}


int CoreBioComponent :: WritePeakInfoToXMLForChannel (int channel, RGTextOutput& text, const RGString& indent, const RGString& tagName) {

	mDataChannels [channel]->WritePeakInfoToXML (text, indent, tagName);
	return 0;
}


int CoreBioComponent :: WriteArtifactInfoToXMLForChannel (int channel, RGTextOutput& text, const RGString& indent) {

	mDataChannels [channel]->WriteArtifactInfoToXML (text, indent, mLSData);
	return 0;
}


int CoreBioComponent :: WriteLocusInfoToXML (RGTextOutput& text, const RGString& indent) {

	for (int i=1; i<=mNumberOfChannels; i++) {

		if (i != mLaneStandardChannel)
			mDataChannels [i]->WriteLocusInfoToXML (text, indent);
	}

	return 0;
}


bool CoreBioComponent :: TestForOffScale (double time) {

	if ((OffScaleData == NULL) || (OffScaleDataLength == 0))
		return false;

	int low = (int) floor (time);
	int high = (int) floor (time + 0.5);
	int low1 = low - 1;
	int high1 = high + 1;
	bool lowOffScale = false;
	bool highOffScale = false;
	bool low1OffScale = false;
	bool high1OffScale = false;

	if ((low >= 0) && (low < OffScaleDataLength)) {

		if (OffScaleData [low])
			lowOffScale = true;
	}

	if ((high >= 0) && (high < OffScaleDataLength)) {

		if (OffScaleData [high])
			highOffScale = true;
	}

	if ((low1 >= 0) && (low1 < OffScaleDataLength)) {

		if (OffScaleData [low1])
			low1OffScale = true;
	}

	if ((high1 >= 0) && (high1 < OffScaleDataLength)) {

		if (OffScaleData [high1])
			high1OffScale = true;
	}

	return (lowOffScale || highOffScale || low1OffScale || high1OffScale);
}


int CoreBioComponent :: InitializeOffScaleData (SampleData& sd) {

	int nPoints;
	const long* temp = sd.GetOffScaleData (nPoints);
	int i;
	int j;

	if (temp == NULL) {

		cout << "Could not retrieve offscale data for sample" << endl;
		OffScaleData = NULL;
		OffScaleDataLength = 0;
		return -1;
	}

	if (nPoints <= 0)
		OffScaleData = NULL;

	else {

		OffScaleData = new bool [OffScaleDataLength];

		for (i=0; i<OffScaleDataLength; i++)
			OffScaleData [i] = false;

		for (i=0; i<nPoints; i++) {

			j = (int)temp [i];

			if ((j >= 0) && (j < OffScaleDataLength))
				OffScaleData [j] = true;
		}
	}

	return 0;
}


void CoreBioComponent :: ReleaseOffScaleData () {

	delete [] OffScaleData;
	OffScaleData = NULL;
	OffScaleDataLength = 0;
}


CSplineTransform* TimeTransform (const CoreBioComponent& cd1, const CoreBioComponent& cd2) {

	return TimeTransform (*cd1.mLSData, *cd2.mLSData);
}


CSplineTransform* TimeTransform (const CoreBioComponent& cd1, const CoreBioComponent& cd2, bool useHermiteSplines) {

	if (!useHermiteSplines)
		return TimeTransform (cd1, cd2);

	CoordinateTransform* id1 = cd1.mLSData->GetIDMap ();
	CoordinateTransform* id2 = cd2.mLSData->GetIDMap ();
	double* firstDerivs1;
	double* firstDerivs2;

	int N1 = id1->GetFirstDerivativeAtKnots (firstDerivs1);
	int N2 = id2->GetFirstDerivativeAtKnots (firstDerivs2);

	if ((N1 == 0) || (N1 != N2)) {

		cout << "Knot mismatch for sample and ladder:  N1 = " << N1 << " N2 = " << N2 << endl;
		delete[] firstDerivs1;
		delete[] firstDerivs2;
		return TimeTransform (cd1, cd2);
	}

	int i;
	double* m = new double [N1];

	for (i=0; i<N1; i++) {

		if (firstDerivs2 [i] <= 0) {

			cout << "First derivative of time-to-bp map is non-positive:  i = " << i << endl;
			delete[] firstDerivs1;
			delete[] firstDerivs2;
			delete[] m;
			return TimeTransform (cd1, cd2);
		}
	}

	for (i=0; i<N1; i++)
		m [i] = firstDerivs1 [i] / firstDerivs2 [i];

	CSplineTransform* spline = TimeTransform (*cd1.mLSData, *cd2.mLSData, m, N1);
	delete[] firstDerivs1;
	delete[] firstDerivs2;
	delete[] m;

	return spline;
}



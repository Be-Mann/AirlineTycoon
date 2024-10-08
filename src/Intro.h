#pragma once
// Intro.cpp : Der Render-Intro

#include "CVideo.h"
#include "defines.h"
#include "Sbbm.h"

/////////////////////////////////////////////////////////////////////////////
// CIntro window
class CIntro : public CVideo {
    // Construction
  public:
    CIntro();
    virtual ~CIntro();

    void OnVideoCancel() override;

    // Attributes
  private:
    GfxLib *mRoomLib{}; // Library für den Raum

    SBBM mFadeFrom;
    SBBM mFadeTo;
    SLONG mFadeCount{};

  public:
    //{{AFX_MSG(COutro)
    void OnPaint() override;
    //}}AFX_MSG
    // DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

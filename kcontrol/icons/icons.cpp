/* vi: ts=8 sts=4 sw=4
 *
 * This file is part of the KDE project, module kcmdisplay.
 * Copyright (C) 2000 Geert Jansen <jansen@kde.org>
 * with minor additions and based on ideas from
 * Torsten Rahn <torsten@kde.org>                                                *
 *
 * You can Freely distribute this program under the GNU General Public
 * License. See the file "COPYING" for the exact licensing terms.
 */

#include <stdlib.h>

#include <QCheckBox>
#include <QComboBox>
#include <QLabel>
#include <QLayout>
#include <QFormLayout>
#include <QSlider>
#include <QGroupBox>
//Added by qt3to4:
#include <QPixmap>
#include <QGridLayout>
#include <QBoxLayout>
#include <QVBoxLayout>
#include <QListWidget>
#include <KApplication>
#include <kcolorbutton.h>
#include <kconfig.h>
#include <kdebug.h>
#include <kiconeffect.h>
#include <kiconloader.h>
#include <klocale.h>
#include <kseparator.h>
#include <kglobalsettings.h>

#include "icons.h"

/**** KIconConfig ****/

KIconConfig::KIconConfig(const KComponentData &inst, QWidget *parent)
    : KCModule(inst, parent)
{

    QGridLayout *top = new QGridLayout(this );
    top->setColumnStretch(0, 1);
    top->setColumnStretch(1, 1);

    // Use of Icon at (0,0) - (1, 0)
    QGroupBox *gbox = new QGroupBox(i18n("Use of Icon"), this);
    top->addWidget(gbox, 0, 0, 2, 1);
    QBoxLayout *g_vlay = new QVBoxLayout(gbox);
    mpUsageList = new QListWidget(gbox);
    connect(mpUsageList, SIGNAL(currentRowChanged(int)), SLOT(slotUsage(int)));
    g_vlay->addWidget(mpUsageList);

    KSeparator *sep = new KSeparator( Qt::Horizontal, this );
    top->addWidget(sep, 1, 1);
    // Preview at (2,0) - (2, 1)
    QGridLayout *g_lay = new QGridLayout();
    g_lay->setSpacing( 0);
    top->addLayout(g_lay, 2, 0, 1, 2 );
    g_lay->addItem(new QSpacerItem(0, fontMetrics().lineSpacing()), 0, 0);

    QPushButton *push;

    push = addPreviewIcon(0, i18nc("@label The icon rendered by default", "Default"), this, g_lay);
    connect(push, SIGNAL(clicked()), SLOT(slotEffectSetup0()));
    push = addPreviewIcon(1, i18nc("@label The icon rendered as active", "Active"), this, g_lay);
    connect(push, SIGNAL(clicked()), SLOT(slotEffectSetup1()));
    push = addPreviewIcon(2, i18nc("@label The icon rendered as disabled", "Disabled"), this, g_lay);
    connect(push, SIGNAL(clicked()), SLOT(slotEffectSetup2()));

    m_pTab1 = new QWidget(this);
    m_pTab1->setObjectName("General Tab");
    top->addWidget(m_pTab1, 0, 1);

    QGridLayout *grid = new QGridLayout(m_pTab1);
    grid->setColumnStretch(1, 1);
    grid->setColumnStretch(2, 1);


    // Size
    QLabel *lbl = new QLabel(i18n("Size:"), m_pTab1);
    lbl->setFixedSize(lbl->sizeHint());
    grid->addWidget(lbl, 0, 0, Qt::AlignLeft);
    mpSizeBox = new QComboBox(m_pTab1);
    connect(mpSizeBox, SIGNAL(activated(int)), SLOT(slotSize(int)));
    lbl->setBuddy(mpSizeBox);
    grid->addWidget(mpSizeBox, 0, 1, Qt::AlignLeft);

    mpAnimatedCheck = new QCheckBox(i18n("Animate icons"), m_pTab1);
    connect(mpAnimatedCheck, SIGNAL(toggled(bool)), SLOT(slotAnimatedCheck(bool)));
    grid->addWidget(mpAnimatedCheck, 2, 0, 1, 2, Qt::AlignLeft);
    grid->setRowStretch(3, 10);

    top->activate();

    init();
    read();
    apply();
    preview();
}

KIconConfig::~KIconConfig()
{
  delete mpEffect;
}

QPushButton *KIconConfig::addPreviewIcon(int i, const QString &str, QWidget *parent, QGridLayout *lay)
{
    QLabel *lab = new QLabel(str, parent);
    lay->addWidget(lab, 1, i, Qt::AlignCenter);
    mpPreview[i] = new QLabel(parent);
    mpPreview[i]->setAlignment(Qt::AlignCenter);
    mpPreview[i]->setMinimumSize(105, 105);
    lay->addWidget(mpPreview[i], 2, i);
    QPushButton *push = new QPushButton(i18n("Set Effect..."), parent);
    lay->addWidget(push, 3, i, Qt::AlignCenter);
    return push;
}

void KIconConfig::init()
{
    mpLoader = KIconLoader::global();
    mpConfig = KGlobal::config();
    mpEffect = new KIconEffect;
    mUsage = 0;
    for (int i=0; i<KIconLoader::LastGroup; i++)
	mbChanged[i] = false;

    // Fill list/checkboxen
    mpUsageList->addItem(i18n("Desktop"));
    mpUsageList->addItem(i18n("Toolbar"));
    mpUsageList->addItem(i18n("Main Toolbar"));
    mpUsageList->addItem(i18n("Small Icons"));
    mpUsageList->addItem(i18n("Panel"));
    mpUsageList->addItem(i18n("All Icons"));

    // For reading the configuration
    mGroups += "Desktop";
    mGroups += "Toolbar";
    mGroups += "MainToolbar";
    mGroups += "Small";
    mGroups += "Panel";

    mStates += "Default";
    mStates += "Active";
    mStates += "Disabled";
}

void KIconConfig::initDefaults()
{
    mDefaultEffect[0].type = KIconEffect::NoEffect;
    mDefaultEffect[1].type = KIconEffect::NoEffect;
    mDefaultEffect[2].type = KIconEffect::ToGray;
    mDefaultEffect[0].transparent = false;
    mDefaultEffect[1].transparent = false;
    mDefaultEffect[2].transparent = true;
    mDefaultEffect[0].value = 1.0;
    mDefaultEffect[1].value = 1.0;
    mDefaultEffect[2].value = 1.0;
    mDefaultEffect[0].color = QColor(144,128,248);
    mDefaultEffect[1].color = QColor(169,156,255);
    mDefaultEffect[2].color = QColor(34,202,0);
    mDefaultEffect[0].color2 = QColor(0,0,0);
    mDefaultEffect[1].color2 = QColor(0,0,0);
    mDefaultEffect[2].color2 = QColor(0,0,0);

    const int defDefSizes[] = { 32, 22, 22, 16, 32 };

    KIconLoader::Group i;
    QStringList::ConstIterator it;
    for(it=mGroups.constBegin(), i=KIconLoader::FirstGroup; it!=mGroups.constEnd(); ++it, i++)
    {
	mbChanged[i] = true;
	mbAnimated[i] = false;
	if (mpLoader->theme())
	    mSizes[i] = mpLoader->theme()->defaultSize(i);
	else
	    mSizes[i] = defDefSizes[i];

	mEffects[i][0] = mDefaultEffect[0];
	mEffects[i][1] = mDefaultEffect[1];
	mEffects[i][2] = mDefaultEffect[2];
    }
    // Animate desktop icons by default
    int group = mGroups.indexOf( "Desktop" );
    if ( group != -1 )
        mbAnimated[group] = true;

    // This is the new default in KDE 2.2, in sync with the kiconeffect of kdelibs Nolden 2001/06/11
    int activeState = mStates.indexOf( "Active" );
    if ( activeState != -1 )
    {
        int group = mGroups.indexOf( "Desktop" );
        if ( group != -1 )
        {
            mEffects[ group ][ activeState ].type = KIconEffect::ToGamma;
            mEffects[ group ][ activeState ].value = 0.7;
        }

        group = mGroups.indexOf( "Panel" );
        if ( group != -1 )
        {
            mEffects[ group ][ activeState ].type = KIconEffect::ToGamma;
            mEffects[ group ][ activeState ].value = 0.7;
        }
    }
}

void KIconConfig::read()
{
    if (mpLoader->theme())
    {
        for (KIconLoader::Group i=KIconLoader::FirstGroup; i<KIconLoader::LastGroup; i++)
            mAvSizes[i] = mpLoader->theme()->querySizes(i);

        mTheme = mpLoader->theme()->current();
        mExample = mpLoader->theme()->example();
    }
    else
    {
        for (KIconLoader::Group i=KIconLoader::FirstGroup; i<KIconLoader::LastGroup; i++)
            mAvSizes[i] = QList<int>();

        mTheme.clear();
        mExample.clear();
    }

    initDefaults();

    int i, j, effect;
    QStringList::ConstIterator it, it2;
    for (it=mGroups.constBegin(), i=0; it!=mGroups.constEnd(); ++it, i++)
    {
        mbChanged[i] = false;

        KConfigGroup iconGroup(mpConfig, *it + "Icons");
	mSizes[i] = iconGroup.readEntry("Size", mSizes[i]);
	mbAnimated[i] = iconGroup.readEntry("Animated", mbAnimated[i]);

	for (it2=mStates.constBegin(), j=0; it2!=mStates.constEnd(); ++it2, j++)
	{
	    QString tmp = iconGroup.readEntry(*it2 + "Effect", QString());
	    if (tmp == "togray")
		effect = KIconEffect::ToGray;
	    else if (tmp == "colorize")
		effect = KIconEffect::Colorize;
	    else if (tmp == "togamma")
		effect = KIconEffect::ToGamma;
	    else if (tmp == "desaturate")
		effect = KIconEffect::DeSaturate;
	    else if (tmp == "tomonochrome")
		effect = KIconEffect::ToMonochrome;
	    else if (tmp == "none")
		effect = KIconEffect::NoEffect;
	    else continue;
	    mEffects[i][j].type = effect;
	    mEffects[i][j].value = iconGroup.readEntry(*it2 + "Value", 0.0);
	    mEffects[i][j].color = iconGroup.readEntry(*it2 + "Color",QColor());
	    mEffects[i][j].color2 = iconGroup.readEntry(*it2 + "Color2", QColor());
	    mEffects[i][j].transparent = iconGroup.readEntry(*it2 + "SemiTransparent", false);
	}
    }
}

void KIconConfig::apply()
{
    mpUsageList->setCurrentRow(mUsage);

    int delta = 1000, dw, index = -1, size = 0, i;
    QList<int>::Iterator it;
    mpSizeBox->clear();
    if (mUsage < KIconLoader::LastGroup) {
        for (it=mAvSizes[mUsage].begin(), i=0; it!=mAvSizes[mUsage].end(); ++it, i++)
        {
            mpSizeBox->addItem(QString().setNum(*it));
            dw = abs(mSizes[mUsage] - *it);
            if (dw < delta)
            {
                delta = dw;
                index = i;
                size = *it;
            }

        }
        if (index != -1)
        {
            mpSizeBox->setCurrentIndex(index);
            mSizes[mUsage] = size; // best or exact match
        }
        mpAnimatedCheck->setChecked(mbAnimated[mUsage]);
    }
}

void KIconConfig::preview(int i)
{
    // Apply effects ourselves because we don't want to sync
    // the configuration every preview.

    int viewedGroup = (mUsage == KIconLoader::LastGroup) ? KIconLoader::FirstGroup : mUsage;

    QPixmap pm = mpLoader->loadIcon(mExample, KIconLoader::NoGroup, mSizes[viewedGroup]);
    QImage img = pm.toImage();

    Effect &effect = mEffects[viewedGroup][i];

    img = mpEffect->apply(img, effect.type,
	    effect.value, effect.color, effect.color2, effect.transparent);
    pm = QPixmap::fromImage(img);
    mpPreview[i]->setPixmap(pm);
}

void KIconConfig::preview()
{
    preview(0);
    preview(1);
    preview(2);
}

void KIconConfig::load()
{
    read();
    apply();
    emit changed(false);
    for (int i=0; i<KIconLoader::LastGroup; i++)
	mbChanged[i] = false;
    preview();
}


void KIconConfig::save()
{
    int i, j;
    QStringList::ConstIterator it, it2;
    for (it=mGroups.constBegin(), i=0; it!=mGroups.constEnd(); ++it, i++)
    {
	KConfigGroup cg(mpConfig, *it + "Icons");
	cg.writeEntry("Size", mSizes[i], KConfig::Normal|KConfig::Global);
	cg.writeEntry("Animated", mbAnimated[i], KConfig::Normal|KConfig::Global);
	for (it2=mStates.constBegin(), j=0; it2!=mStates.constEnd(); ++it2, j++)
	{
	    QString tmp;
	    switch (mEffects[i][j].type)
	    {
	    case KIconEffect::ToGray:
		tmp = "togray";
		break;
	    case KIconEffect::ToGamma:
		tmp = "togamma";
		break;
	    case KIconEffect::Colorize:
		tmp = "colorize";
		break;
	    case KIconEffect::DeSaturate:
		tmp = "desaturate";
		break;
	    case KIconEffect::ToMonochrome:
		tmp = "tomonochrome";
		break;
	    default:
		tmp = "none";
		break;
	    }
	    cg.writeEntry(*it2 + "Effect", tmp, KConfig::Normal|KConfig::Global);
	    cg.writeEntry(*it2 + "Value", mEffects[i][j].value, KConfig::Normal|KConfig::Global);
            cg.writeEntry(*it2 + "Color", mEffects[i][j].color, KConfig::Normal|KConfig::Global);
            cg.writeEntry(*it2 + "Color2", mEffects[i][j].color2, KConfig::Normal|KConfig::Global);
            cg.writeEntry(*it2 + "SemiTransparent", mEffects[i][j].transparent, KConfig::Normal|KConfig::Global);
	}
    }

    mpConfig->sync();

    emit changed(false);

    // Emit KIPC change message.
    for (int i=0; i<KIconLoader::LastGroup; i++)
    {
	if (mbChanged[i])
	{
	    KGlobalSettings::self()->emitChange(KGlobalSettings::IconChanged, i);
	    mbChanged[i] = false;
	}
    }
}

void KIconConfig::defaults()
{
    initDefaults();
    apply();
    preview();
    emit changed(true);
}

void KIconConfig::slotUsage(int index)
{
    if (index == -1)
        return;

    mUsage = index;
    if ( mUsage == KIconLoader::Panel || mUsage == KIconLoader::LastGroup )
    {
        mpSizeBox->setEnabled(false);
	mpAnimatedCheck->setEnabled( mUsage == KIconLoader::Panel );
    }
    else
    {
        mpSizeBox->setEnabled(true);
	mpAnimatedCheck->setEnabled( mUsage == KIconLoader::Desktop );
    }

    apply();
    preview();
}

void KIconConfig::EffectSetup(int state)
{
    int viewedGroup = (mUsage == KIconLoader::LastGroup) ? KIconLoader::FirstGroup : mUsage;

    QPixmap pm = mpLoader->loadIcon(mExample, KIconLoader::NoGroup, mSizes[viewedGroup]);
    QImage img = pm.toImage();

    QString caption;
    switch (state)
    {
    case 0 : caption = i18n("Setup Default Icon Effect"); break;
    case 1 : caption = i18n("Setup Active Icon Effect"); break;
    case 2 : caption = i18n("Setup Disabled Icon Effect"); break;
    }

    KIconEffectSetupDialog dlg(mEffects[viewedGroup][state], mDefaultEffect[state], caption, img, this);

    if (dlg.exec() == QDialog::Accepted)
    {
        if (mUsage == KIconLoader::LastGroup) {
            for (int i=0; i<KIconLoader::LastGroup; i++)
                mEffects[i][state] = dlg.effect();
        } else {
            mEffects[mUsage][state] = dlg.effect();
        }

        // AK - can this call be moved therefore removing
        //      code duplication?

        emit changed(true);

        if (mUsage == KIconLoader::LastGroup) {
            for (int i=0; i<KIconLoader::LastGroup; i++)
                mbChanged[i] = true;
        } else {
            mbChanged[mUsage] = true;
        }
    }
    preview(state);
}

void KIconConfig::slotSize(int index)
{
    Q_ASSERT(mUsage < KIconLoader::LastGroup);
    mSizes[mUsage] = mAvSizes[mUsage][index];
    preview();
    emit changed(true);
    mbChanged[mUsage] = true;
}

void KIconConfig::slotAnimatedCheck(bool check)
{
    Q_ASSERT(mUsage < KIconLoader::LastGroup);
    if (mbAnimated[mUsage] != check)
    {
        mbAnimated[mUsage] = check;
        emit changed(true);
        mbChanged[mUsage] = true;
    }
}

KIconEffectSetupDialog::KIconEffectSetupDialog(const Effect &effect,
    const Effect &defaultEffect,
    const QString &caption, const QImage &image,
    QWidget *parent, char *name)
    : KDialog( parent ),
      mEffect(effect),
      mDefaultEffect(defaultEffect),
      mExample(image)
{
    setObjectName( name );
    setModal( true );
    setCaption( caption );
    setButtons( Default|Ok|Cancel );
    showButtonSeparator( true );

    mpEffect = new KIconEffect;

    QLabel *lbl;
    QGroupBox *frame;
    QGridLayout *grid;

    QWidget *page = new QWidget(this);
    setMainWidget(page);

    QGridLayout *top = new QGridLayout(page);
    top->setMargin(0);
    top->setColumnStretch(0,1);
    top->setColumnStretch(1,2);
    top->setRowStretch(1,1);

    lbl = new QLabel(i18n("&Effect:"), page);
    top->addWidget(lbl, 0, 0, Qt::AlignLeft);
    mpEffectBox = new QListWidget(page);
    mpEffectBox->addItem(i18n("No Effect"));
    mpEffectBox->addItem(i18n("To Gray"));
    mpEffectBox->addItem(i18n("Colorize"));
    mpEffectBox->addItem(i18n("Gamma"));
    mpEffectBox->addItem(i18n("Desaturate"));
    mpEffectBox->addItem(i18n("To Monochrome"));
    connect(mpEffectBox, SIGNAL(currentRowChanged(int)), SLOT(slotEffectType(int)));
    top->addWidget(mpEffectBox, 1, 0, 2, 1, Qt::AlignLeft);
    lbl->setBuddy(mpEffectBox);

    mpSTCheck = new QCheckBox(i18n("&Semi-transparent"), page);
    connect(mpSTCheck, SIGNAL(toggled(bool)), SLOT(slotSTCheck(bool)));
    top->addWidget(mpSTCheck, 3, 0, Qt::AlignLeft);

    frame = new QGroupBox(i18n("Preview"), page);
    top->addWidget(frame, 0, 1, 2, 1);
    grid = new QGridLayout(frame);
    grid->addItem(new QSpacerItem(0, fontMetrics().lineSpacing()), 0, 0);
    grid->setRowStretch(1, 1);

    mpPreview = new QLabel(frame);
    mpPreview->setAlignment(Qt::AlignCenter);
    mpPreview->setMinimumSize(105, 105);
    grid->addWidget(mpPreview, 1, 0);

    mpEffectGroup = new QGroupBox(i18n("Effect Parameters"), page);
    top->addWidget(mpEffectGroup, 2, 1, 2, 1);
    QFormLayout *form = new QFormLayout(mpEffectGroup);

    mpEffectSlider = new QSlider(Qt::Horizontal, mpEffectGroup);
    mpEffectSlider->setMinimum(0);
    mpEffectSlider->setMaximum(100);
    mpEffectSlider->setPageStep(5);
    connect(mpEffectSlider, SIGNAL(valueChanged(int)), SLOT(slotEffectValue(int)));
    form->addRow(i18n("&Amount:"), mpEffectSlider);
    mpEffectLabel = static_cast<QLabel *>(form->labelForField(mpEffectSlider));

    mpEColButton = new KColorButton(mpEffectGroup);
    connect(mpEColButton, SIGNAL(changed(const QColor &)),
		SLOT(slotEffectColor(const QColor &)));
    form->addRow(i18n("Co&lor:"), mpEColButton);
    mpEffectColor = static_cast<QLabel *>(form->labelForField(mpEColButton));

    mpECol2Button = new KColorButton(mpEffectGroup);
    connect(mpECol2Button, SIGNAL(changed(const QColor &)),
		SLOT(slotEffectColor2(const QColor &)));
    form->addRow(i18n("&Second color:"), mpECol2Button);
    mpEffectColor2 = static_cast<QLabel *>(form->labelForField(mpECol2Button));

    init();
    preview();
}

KIconEffectSetupDialog::~KIconEffectSetupDialog()
{
  delete mpEffect;
}

void KIconEffectSetupDialog::init()
{
    mpEffectBox->setCurrentRow(mEffect.type);
    mpEffectSlider->setEnabled(mEffect.type != KIconEffect::NoEffect);
    mpEColButton->setEnabled(mEffect.type == KIconEffect::Colorize || mEffect.type == KIconEffect::ToMonochrome);
    mpECol2Button->setEnabled(mEffect.type == KIconEffect::ToMonochrome);
    mpEffectSlider->setValue((int) (100.0 * mEffect.value + 0.5));
    mpEColButton->setColor(mEffect.color);
    mpECol2Button->setColor(mEffect.color2);
    mpSTCheck->setChecked(mEffect.transparent);
}

void KIconEffectSetupDialog::slotEffectValue(int value)
{
     mEffect.value = 0.01 * value;
     preview();
}

void KIconEffectSetupDialog::slotEffectColor(const QColor &col)
{
     mEffect.color = col;
     preview();
}

void KIconEffectSetupDialog::slotEffectColor2(const QColor &col)
{
     mEffect.color2 = col;
     preview();
}

void KIconEffectSetupDialog::slotEffectType(int type)
{
    if (type == -1)
        return;

    mEffect.type = type;
    mpEffectGroup->setEnabled(mEffect.type != KIconEffect::NoEffect);
    mpEffectSlider->setEnabled(mEffect.type != KIconEffect::NoEffect);
    mpEffectColor->setEnabled(mEffect.type == KIconEffect::Colorize || mEffect.type == KIconEffect::ToMonochrome);
    mpEColButton->setEnabled(mEffect.type == KIconEffect::Colorize || mEffect.type == KIconEffect::ToMonochrome);
    mpEffectColor2->setEnabled(mEffect.type == KIconEffect::ToMonochrome);
    mpECol2Button->setEnabled(mEffect.type == KIconEffect::ToMonochrome);
    preview();
}

void KIconEffectSetupDialog::slotSTCheck(bool b)
{
     mEffect.transparent = b;
     preview();
}

void KIconEffectSetupDialog::slotDefault()
{
     mEffect = mDefaultEffect;
     init();
     preview();
}

void KIconEffectSetupDialog::preview()
{
    QPixmap pm;
    QImage img = mExample.copy();
    img = mpEffect->apply(img, mEffect.type,
          mEffect.value, mEffect.color, mEffect.color2, mEffect.transparent);
    pm  = QPixmap::fromImage(img);
    mpPreview->setPixmap(pm);
}

#include "icons.moc"

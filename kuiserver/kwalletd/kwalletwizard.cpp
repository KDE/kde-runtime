#include "kwalletwizard.h"
#include "kwalletwizard.moc"
KWalletWizard::KWalletWizard( QWidget *parent )
  : Q3Wizard( parent )
{
  setupUi( this );
  connect(_useWallet, SIGNAL(toggled(bool)),textLabel1_2,SLOT(setEnabled(bool)));
  connect(_useWallet, SIGNAL(toggled(bool)),textLabel2_2,SLOT(setEnabled(bool)));
  connect(_useWallet, SIGNAL(toggled(bool)),_pass1,SLOT(setEnabled(bool)));
  connect(_useWallet, SIGNAL(toggled(bool)),_pass2,SLOT(setEnabled(bool)));
  connect(_useWallet, SIGNAL(clicked()), _pass1,SLOT(setFocus()));
  connect(_useWallet, SIGNAL(clicked()), this, SLOT(passwordPageUpdate()));
  connect(_pass1, SIGNAL(textChanged(const QString&)),this,SLOT(passwordPageUpdate()));
  connect(_pass2, SIGNAL(textChanged(const QString&)),this,SLOT(passwordPageUpdate()));
  connect(_advanced,SIGNAL(clicked()),this,SLOT(setAdvanced()));
  connect(_basic,SIGNAL(clicked()),this, SLOT(setBasic()));

}

void KWalletWizard::passwordPageUpdate()
{
    bool fe = !_useWallet->isChecked() || _pass1->text() == _pass2->text();
    if (_basic->isChecked()) {
        setFinishEnabled(page2, fe);
    } else {
        setNextEnabled(page2, fe);
        setFinishEnabled(page3, fe);
    }

    if (_useWallet->isChecked()) {
        if (_pass1->text() == _pass2->text()) {
            if (_pass1->text().isEmpty()) {
                _matchLabel->setText(i18n("<qt>Password is empty.  <b>(WARNING: Insecure)"));
            } else {
                _matchLabel->setText(i18n("Passwords match."));
            }
        } else {
            _matchLabel->setText(i18n("Passwords do not match."));
        }
    } else {
        _matchLabel->setText(QString());
    }
}

void KWalletWizard::init()
{
    setHelpEnabled(page1, false);
    setHelpEnabled(page2, false);
    setHelpEnabled(page3, false);
    setHelpEnabled(page4, false);
    setAppropriate(page3, false);
    setAppropriate(page4, false);
    setFinishEnabled(page2, true);
}

void KWalletWizard::setAdvanced()
{
    setAppropriate(page3, true);
    setAppropriate(page4, true);
    bool fe = !_useWallet->isChecked() || _pass1->text() == _pass2->text();
    setFinishEnabled(page2, false);
    setNextEnabled(page2, fe);
    setFinishEnabled(page3, fe);
}

void KWalletWizard::setBasic()
{
    setAppropriate(page3, false);
    setAppropriate(page4, false);
    bool fe = !_useWallet->isChecked() || _pass1->text() == _pass2->text();
    setFinishEnabled(page3, false);
    setFinishEnabled(page2, fe);
}

void KWalletWizard::destroy()
{
    _pass1->clear();
    _pass2->clear();
}

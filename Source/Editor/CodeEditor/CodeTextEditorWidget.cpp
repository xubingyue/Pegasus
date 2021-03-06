/****************************************************************************************/
/*                                                                                      */
/*                                    Pegasus Editor                                    */
/*                                                                                      */
/****************************************************************************************/

//! \file	CodeTextEditorWidget.h
//! \author	Kleber Garcia
//! \date	22nd April 2014
//! \brief	Graphics widget representing a single source code text editor
#include "CodeEditor/CodeTextEditorWidget.h"
#include "CodeEditor/SourceCodeManagerEventListener.h"
#include "Pegasus/Core/Shared/ISourceCodeProxy.h"
#include "Pegasus/Application/Shared/IApplicationProxy.h"
#include "Pegasus/Version.h"
#include "Application/Application.h"
#include "Application/ApplicationManager.h"

#include <QEvent>
#include <QHelpEvent>
#include <QString>
#include <Qtooltip.h>
#include <QSyntaxHighlighter>
#include <QTextDocument>
#include <QTextCharFormat>
#include <QVector>
#include <QSet>
#include <QPainter>


//! internal class serving as the syntax highlighter for shade code
class CodeSyntaxHighlighter : public QSyntaxHighlighter
{
public:
    CodeSyntaxHighlighter(QTextDocument * parent)
    : QSyntaxHighlighter(parent), mSignalSyntaxError(false), mSourceState(nullptr)
    {
    }

    virtual ~CodeSyntaxHighlighter() {}

    void SetSourceState(const SourceState * sourceState) { mSourceState = sourceState;}

    void SetSyntaxLanguage(Pegasus::PegasusDesc::GapiType gapi)
    {
        static const char  * glslKeywords[] = {
            "\\b[0-9]*\\.?[0-9]+f?\\b",
            "\\bsin\\b",
            "\\bcos\\b",
            "\\bmix\\b",
            "\\bdot\\b",
            "\\bnormalize\\b",
            "\\bdistance\\b",
            "\\bvoid\\b",
            "\\bfloat\\b",
            "\\b[biud]?vec[1-4]?\\b",
            "\\bbool\\b",
            "\\bint\\b",
            "\\buint\\b",
            "\\bfloat\\b",
            "\\bdouble\\b",
            "\\bmat[1-4]x?[1-4]?\\b",
            "\\buniform\\b",
            "\\bvarying\\b",
            "\\battribute\\b",
            "\\bstruct\\b",
            "\\bout\\b",
            "\\blayout\\b",
            "\\bin\\b",
            "\\bpow\\b",
            "\\bout\\b",
            nullptr
        };

        static const char  * hlslKeywords[] = {
            "\\b[0-9]*\\.?[0-9]+f?\\b",
            "\\bsin\\b",
            "\\bcos\\b",
            "\\bsincos\\b",
            "\\blerp\\b",
            "\\bdot\\b",
            "\\bnormalize\\b",
            "\\bdistance\\b",
            "\\bvoid\\b",
            "\\bfloat[1-4]?\\b",
            "\\bbool\\b",
            "\\bint\\b",
            "\\buint\\b",
            "\\bfloat\\b",
            "\\bdouble\\b",
            "\\bfloat[1-4]x?[1-4]?\\b",
            "\\buniform\\b",
            "\\bregister\\b",
            "\\bcbuffer\\b",
            "\\bTexture[123]D\\b",
            "\\bSamplerState\\b",
            "\\bSample\\b",
            "\\bSampleLevel\\b",
            "\\bGatherRed\\b",
            "\\bGatherGreen\\b",
            "\\bGatherBlue\\b",
            "\\bGatherAlpha\\b",
            "\\bSV_Position\\b",
            "\\bPOSITION[0-9]\\b",
            "\\bTEXCOORD[0-9]\\b",
            "\\bCOLOR[0-9]\\b",
            "\\bNORAL[0-9]\\b",
            "\\bBINORAL[0-9]\\b",
            "\\bTANGENT[0-9]\\b",
            "\\bSV_Target[0-9]?\\b",
            "\\bclamp\\b",
            "\\bsaturate\\b",
            "\\bstep\\b",
            "\\bin\\b",
            "\\bout\\b",
            nullptr
        };

        const char ** keywords = nullptr;

        if (gapi == Pegasus::PegasusDesc::OPEN_GL || gapi == Pegasus::PegasusDesc::OPEN_GLES)
        {
            keywords = glslKeywords;
        }
        else if (gapi == Pegasus::PegasusDesc::DIRECT_3D)
        {
            keywords = hlslKeywords;
        }
        else
        {
            ED_FAILSTR("No graphics library defined :(");
        }

        mRules.clear();
        const char * ptr = keywords[0];
        int i = 0;
        QTextCharFormat keywordFormat;
        while (ptr != nullptr)
        {
            QRegExp keywordExp(ptr);
            mRules << keywordExp;
            ptr = keywords[++i]; 
        }
    }
    
    
protected:

    QVector<QRegExp> mRules;
    
    bool mWrongLines;
    bool mSignalSyntaxError;
    const SourceState * mSourceState;

    //! sets the formats for comments
    void SetCCommentStyle(int start, int end, Settings * settings)
    {
        QTextCharFormat f;
        f.setForeground(settings->GetCodeSyntaxColor(Settings::SYNTAX_C_COMMENT));
        if (mSignalSyntaxError) f.setUnderlineStyle(QTextCharFormat::WaveUnderline);
        setFormat(start, end, f);
    }

    //! sets the formats for CPP coments
    void SetCPPCommentStyle(int start, int end, Settings * settings)
    {
        QTextCharFormat f;
        f.setForeground(settings->GetCodeSyntaxColor(Settings::SYNTAX_CPP_COMMENT));
        if (mSignalSyntaxError) f.setUnderlineStyle(QTextCharFormat::WaveUnderline);
        setFormat(start, end, f);
    }

    //! sets the formats for regular style
    void SetNormalStyle(int start, int end, Settings * settings)
    {
        QTextCharFormat f;
        f.setForeground(settings->GetCodeSyntaxColor(Settings::SYNTAX_NORMAL_TEXT));
        if (mSignalSyntaxError) f.setUnderlineStyle(QTextCharFormat::WaveUnderline);
        setFormat(start, end, f);
    }

    //! qt callback that processes highlighting in a line of text
    virtual void highlightBlock(const QString& text)
    {
        //TODO: Because the settings has a direct dependency on the widget dock, 
        //      we must request the settings singleton every time we highlight a line.
        //      this is a lot of memory dereference :/ we could optimize this if we cache
        //      the settings as a member of this class
        Settings * settings = Editor::GetInstance().GetSettings();
        // state machine definitions
        const int NormalState = -1;
        const int CommentState = 0;
        int state = previousBlockState();
        int start = 0;
        bool isCommentLine = false;

        mSignalSyntaxError = mSourceState == nullptr ? false : mSourceState->errorLines.contains(currentBlock().firstLineNumber()+1);

        // for every character
        for (int i = 0; i < text.length(); ++i)
        {
            if (state == CommentState)
            {
                //close the comment
                if (text.mid(i, 2) == "*/")
                {
                    state = NormalState;
                    SetCPPCommentStyle(start, i - start + 2, settings);
                    start = i + 2;
                }
            }
            else
            {
                // trigger a full line comment
                if (text.mid(i, 2) == "//")
                {
                    isCommentLine = true;
                    SetCCommentStyle(i, text.length() - i, settings);
                    start = text.length();
                    break;
                }
                // trigger a comment block
                else if (text.mid(i, 2) == "/*")
                {
                    start = i;
                    state = CommentState; 
                }
            }
        }

        if (state == CommentState && start < text.length())
        {
            SetCPPCommentStyle(start, text.length() - start, settings);
            start =  text.length();
        }

        if (start < text.length())
        {
            SetNormalStyle(start, text.length(), settings);
        }

        if (state != CommentState && !isCommentLine)
        {
            for (int i = 0; i < mRules.size(); ++i)
            {
                QRegExp& pattern = mRules[i];
                int index = pattern.indexIn(text);
                while (index >= 0)
                {
                    int length = pattern.matchedLength();
                    if (start < index + length)
                    {
                        start = index + length;
                    }
                    Settings::CodeEditorSyntaxStyle style = i == 0 ? Settings::SYNTAX_NUMBER_VALUE : Settings::SYNTAX_KEYWORD;
                    QTextCharFormat f;
                    f.setForeground(settings->GetCodeSyntaxColor(style));
                    if (mSignalSyntaxError) f.setUnderlineStyle(QTextCharFormat::WaveUnderline);
                    setFormat(index, length, f);
                    index = pattern.indexIn(text, index + length);
               
                }
            }
            if (start < text.length())
            {
                SetNormalStyle(start, text.length(), settings);
            }
        }
        setCurrentBlockState(state);
    }
    
};

class LineNumberArea : public QWidget
{
public:
    LineNumberArea(CodeTextEditorWidget *editor) : QWidget(editor) 
    {
        codeEditor = editor;
    }

    virtual QSize sizeHint() const 
    {
        return QSize(codeEditor->lineNumberAreaWidth(), 0);
    }

protected:
    virtual void paintEvent(QPaintEvent *event)
    {
        codeEditor->lineNumberAreaPaintEvent(event);
    }

private:
    CodeTextEditorWidget *codeEditor;
};


CodeTextEditorWidget::CodeTextEditorWidget(QWidget * parent)
: QPlainTextEdit(parent), mCode(nullptr), mIsFocus(false)
{
    mSyntaxHighlighter = new CodeSyntaxHighlighter(nullptr);
    mNullDocument = new QTextDocument(this);
    mNullDocument->setDocumentLayout(new QPlainTextDocumentLayout(mNullDocument));
    mLineNumberArea = new LineNumberArea(this);
    connect(this, SIGNAL(blockCountChanged(int)), this, SLOT(updateLineNumberAreaWidth(int)));
    connect(this, SIGNAL(updateRequest(QRect,int)), this, SLOT(updateLineNumberArea(QRect,int)));
    connect(this, SIGNAL(cursorPositionChanged()), this, SLOT(highlightCurrentLine()));

    setDocument(mNullDocument);
    updateLineNumberAreaWidth(0);
}

int CodeTextEditorWidget::lineNumberAreaWidth()
{
    int digits = 1;
    int max = qMax(1, blockCount());
    while (max >= 10) {
        max /= 10;
        ++digits;
    }

    int space = 3 + fontMetrics().width(QLatin1Char('9')) * digits;

    return space;
}

void CodeTextEditorWidget::updateLineNumberAreaWidth(int newBlockCount)
{
    setViewportMargins(lineNumberAreaWidth(), 0, 0, 0);
}

void CodeTextEditorWidget::updateLineNumberArea(const QRect &rect, int dy)
{
    if (dy)
        mLineNumberArea->scroll(0, dy);
    else
        mLineNumberArea->update(0, rect.y(), mLineNumberArea->width(), rect.height());

    if (rect.contains(viewport()->rect()))
        updateLineNumberAreaWidth(0);
}

void CodeTextEditorWidget::resizeEvent(QResizeEvent *e)
{
    QPlainTextEdit::resizeEvent(e);

    QRect cr = contentsRect();
    mLineNumberArea->setGeometry(QRect(cr.left(), cr.top(), lineNumberAreaWidth(), cr.height()));
}

void CodeTextEditorWidget::highlightCurrentLine()
{
    QList<QTextEdit::ExtraSelection> extraSelections;

    if (!isReadOnly() && Editor::GetInstance().GetSettings() != nullptr) {
        QTextEdit::ExtraSelection selection;

        QColor lineColor = Editor::GetInstance().GetSettings()->GetCodeSyntaxColor(Settings::SYNTAX_SELECTED_LINE_BG_COL);

        selection.format.setBackground(lineColor);
        selection.format.setProperty(QTextFormat::FullWidthSelection, true);
        selection.cursor = textCursor();
        selection.cursor.clearSelection();
        extraSelections.append(selection);
    }

    setExtraSelections(extraSelections);
}

void CodeTextEditorWidget::lineNumberAreaPaintEvent(QPaintEvent *event)
{
    QPainter painter(mLineNumberArea);
    if (Editor::GetInstance().GetSettings() != nullptr)
    {
        //TODO: make these color settings
        QColor bgTextCol = Editor::GetInstance().GetSettings()->GetCodeSyntaxColor(Settings::SYNTAX_LINE_NUMBER_BG);
        QColor textCol = Editor::GetInstance().GetSettings()->GetCodeSyntaxColor(Settings::SYNTAX_LINE_NUMBER_COL);
        painter.fillRect(event->rect(), bgTextCol);
        QTextBlock block = firstVisibleBlock();
        int blockNumber = block.blockNumber();
        int top = (int) blockBoundingGeometry(block).translated(contentOffset()).top();
        int bottom = top + (int) blockBoundingRect(block).height();
        while (block.isValid() && top <= event->rect().bottom()) {
            if (block.isVisible() && bottom >= event->rect().top()) {
                QString number = QString::number(blockNumber + 1);
                painter.setPen(textCol);
                painter.drawText(0, top, mLineNumberArea->width(), fontMetrics().height(),
                                 Qt::AlignRight, number);
            }

            block = block.next();
            top = bottom;
            bottom = top + (int) blockBoundingRect(block).height();
            ++blockNumber;
        }
    }
}

CodeTextEditorWidget::~CodeTextEditorWidget()
{
    delete mSyntaxHighlighter;
}

void CodeTextEditorWidget::Initialize(SourceState * code)
{
    mCode = code;
    mSyntaxHighlighter->setDocument(nullptr);
    if (code != nullptr)
    {
        setDocument(code->document);
        Application * application = Editor::GetInstance().GetApplicationManager().GetApplication();
        Pegasus::App::IApplicationProxy * app = application->GetApplicationProxy();
        Pegasus::PegasusDesc::GapiType gapi = Pegasus::PegasusDesc::OPEN_GL;
        if (app != nullptr)
        {
            Pegasus::PegasusDesc desc;
            app->GetEngineDesc(desc);
            gapi = desc.mGapiType;
        }
        else
        {
            ED_FAILSTR("No Gapi defined!");
        }
        mSyntaxHighlighter->setDocument(code->document);
        static_cast<CodeSyntaxHighlighter*>(mSyntaxHighlighter)->SetSyntaxLanguage(gapi);
        static_cast<CodeSyntaxHighlighter*>(mSyntaxHighlighter)->SetSourceState(code); 
        
    
    }
    else
    {
        mNullDocument->clear();
        setDocument(mNullDocument);
   
        static_cast<CodeSyntaxHighlighter*>(mSyntaxHighlighter)->SetSourceState(nullptr);
        setPlainText(tr(""));
    }
    UpdateAllDocumentSyntax();
}

void CodeTextEditorWidget::Uninitialize()
{
    mCode = nullptr;
    static_cast<CodeSyntaxHighlighter*>(mSyntaxHighlighter)->SetSourceState(nullptr);
    
}

void CodeTextEditorWidget::UpdateLineSyntax(int line)
{
    QTextDocument * doc = document();
    int lineId = line - 1;
    if (lineId >= 0 && lineId < doc->lineCount())
    {
        QTextBlock block = doc->findBlockByLineNumber(lineId);
        mSyntaxHighlighter->rehighlightBlock(block);
    }
}

void CodeTextEditorWidget::UpdateAllDocumentSyntax()
{
    QPalette p = palette();
    p.setColor(QPalette::Base, Editor::GetInstance().GetSettings()->GetCodeSyntaxColor(Settings::SYNTAX_BACKGROUND));
    p.setColor(QPalette::Text, Editor::GetInstance().GetSettings()->GetCodeSyntaxColor(Settings::SYNTAX_NORMAL_TEXT));
    int fontSize = Editor::GetInstance().GetSettings()->GetCodeEditorFontSize();
    int tabStop  = Editor::GetInstance().GetSettings()->GetCodeEditorTabSize();

    QFont f("Courier");
    f.setPointSize(fontSize);
    
    QFontMetrics metrics(f);

    document()->setDefaultFont(f);
    setPalette(p);
    setTabStopWidth(tabStop * metrics.width(' '));

    mSyntaxHighlighter->rehighlight();
}

bool CodeTextEditorWidget::event(QEvent * e)
{
    if (e->type() == QEvent::ToolTip)
    {
        if (mCode != nullptr)
        {
            QMap<int, QString>& messageMap = mCode->errorMessages;
            QHelpEvent * helpEvent = static_cast<QHelpEvent*>(e);
            const QPoint& pos = helpEvent->pos();
            QTextCursor cursor = cursorForPosition(pos);
            int line = cursor.block().firstLineNumber() + 1;
            QMap<int, QString>::iterator it = messageMap.find(line);
            if (it != messageMap.end())
            {
                QToolTip::showText(helpEvent->globalPos(), QString(it.value()));
            }
        }
    }
    return QPlainTextEdit::event(e);
}

void CodeTextEditorWidget::focusInEvent(QFocusEvent * e)
{
    mIsFocus = true;
    QPlainTextEdit::focusInEvent(e);
    emit(SignalSelected());
}

void CodeTextEditorWidget::focusOutEvent(QFocusEvent * e)
{
    mIsFocus = false;
    QPlainTextEdit::focusOutEvent(e);
}

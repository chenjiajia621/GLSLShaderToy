#pragma once
#include <QSyntaxHighlighter>
#include <QQuickTextDocument>
#include <QRegularExpression>

// 实际执行高亮逻辑的类
class HighlighterShader : public QSyntaxHighlighter
{
public:
    HighlighterShader(QTextDocument *parent = nullptr);

protected:
    void highlightBlock(const QString &text) override;

private:
    struct HighlightingRule
    {
        QRegularExpression pattern;
        QTextCharFormat format;
    };
    QVector<HighlightingRule> highlightingRules;

    QTextCharFormat keywordFormat;
    QTextCharFormat classFormat;
    QTextCharFormat singleLineCommentFormat;
    QTextCharFormat quotationFormat;
    QTextCharFormat functionFormat;
    QTextCharFormat numberFormat;
};


// 暴露给 QML 的桥接类
class CodeHighlighter : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QQuickTextDocument* document READ document WRITE setDocument NOTIFY documentChanged)
    QML_ELEMENT

public:
    explicit CodeHighlighter(QObject *parent = nullptr);

    QQuickTextDocument *document() const;
    void setDocument(QQuickTextDocument *document);

signals:
    void documentChanged();

private:
    QQuickTextDocument *m_document;
    HighlighterShader *m_highlighter;
};

#include "HighlighterShader.h"
#include <QTextDocument>

HighlighterShader::HighlighterShader(QTextDocument *parent)
    : QSyntaxHighlighter(parent)
{
    HighlightingRule rule;

    // 1. 关键字格式 (比如 vec3, float, void) - 紫色/蓝色
    keywordFormat.setForeground(QColor("#569CD6")); // VS Code 风格蓝
    keywordFormat.setFontWeight(QFont::Bold);
    QStringList keywordPatterns;
    keywordPatterns << "\\bvoid\\b" << "\\bfloat\\b" << "\\bint\\b" << "\\bbool\\b"
                    << "\\bvec2\\b" << "\\bvec3\\b" << "\\bvec4\\b"
                    << "\\bmat2\\b" << "\\bmat3\\b" << "\\bmat4\\b"
                    << "\\btexture\\b" << "\\bmix\\b" << "\\bsmoothstep\\b"
                    << "\\bdot\\b" << "\\bcross\\b" << "\\bnormalize\\b"
                    << "\\bsin\\b" << "\\bcos\\b" << "\\btan\\b"
                    << "\\bmin\\b" << "\\bmax\\b" << "\\bclamp\\b"
                    << "\\bpow\\b" << "\\bsqrt\\b" << "\\blength\\b"
                    << "\\bgl_FragCoord\\b" << "\\bgl_FragColor\\b";

    for (const QString &pattern : keywordPatterns) {
        rule.pattern = QRegularExpression(pattern);
        rule.format = keywordFormat;
        highlightingRules.append(rule);
    }

    // 2. 数字格式 - 浅绿色
    numberFormat.setForeground(QColor("#B5CEA8"));
    rule.pattern = QRegularExpression("\\b\\d+(\\.\\d+)?f?\\b");
    rule.format = numberFormat;
    highlightingRules.append(rule);

    // 3. 函数调用格式 (比如 main() ) - 黄色
    functionFormat.setForeground(QColor("#DCDCAA"));
    rule.pattern = QRegularExpression("\\b[A-Za-z0-9_]+(?=\\()");
    rule.format = functionFormat;
    highlightingRules.append(rule);

    // 4. 注释格式 (// ...) - 绿色
    singleLineCommentFormat.setForeground(QColor("#6A9955"));
    rule.pattern = QRegularExpression("//[^\n]*");
    rule.format = singleLineCommentFormat;
    highlightingRules.append(rule);

    // 5. 宏定义 (#define ...) - 紫色
    classFormat.setForeground(QColor("#C586C0"));
    rule.pattern = QRegularExpression("#[^\n]*");
    rule.format = classFormat;
    highlightingRules.append(rule);
}

void HighlighterShader::highlightBlock(const QString &text)
{
    for (const HighlightingRule &rule : highlightingRules) {
        QRegularExpressionMatchIterator matchIterator = rule.pattern.globalMatch(text);
        while (matchIterator.hasNext()) {
            QRegularExpressionMatch match = matchIterator.next();
            setFormat(match.capturedStart(), match.capturedLength(), rule.format);
        }
    }
}

// ==========================================
// QML 桥接类的实现
// ==========================================

CodeHighlighter::CodeHighlighter(QObject *parent)
    : QObject(parent), m_document(nullptr), m_highlighter(nullptr)
{

}

QQuickTextDocument *CodeHighlighter::document() const
{
    return m_document;
}

void CodeHighlighter::setDocument(QQuickTextDocument *document)
{
    if (document == m_document)
        return;

    m_document = document;

    // 当 QML 传入 document 时，创建 C++ 高亮器并挂载上去
    if (m_document) {
        if (!m_highlighter)
            m_highlighter = new HighlighterShader(m_document->textDocument());
        else
            m_highlighter->setDocument(m_document->textDocument());
    }

    emit documentChanged();
}

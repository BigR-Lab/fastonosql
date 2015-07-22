#pragma once

#include <Qsci/qsciabstractapis.h>
#include <Qsci/qscilexercustom.h>

#define ALL_COMMANDS "ALL_COMMANDS"

namespace fastonosql
{
    class SsdbApi
            : public QsciAbstractAPIs
    {
        Q_OBJECT
    public:
        SsdbApi(QsciLexer* lexer);

        virtual void updateAutoCompletionList(const QStringList& context, QStringList& list);
        virtual QStringList callTips(const QStringList& context, int commas, QsciScintilla::CallTipsStyle style, QList<int>& shifts);
    };

    class SsdbLexer
            : public QsciLexerCustom
    {
        Q_OBJECT
    public:
        enum
        {
            Default = 0,
            Command = 1,
            HelpKeyword
        };

        SsdbLexer(QObject* parent = 0);
        virtual const char* language() const;
        static const char* version();

        virtual QString description(int style) const;
        virtual void styleText(int start, int end);
        virtual QColor defaultColor(int style) const;

    private:
        void paintCommands(const QString& source, int start);
    };
}

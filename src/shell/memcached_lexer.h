#pragma once

#include "shell/base_lexer.h"

namespace fastonosql
{
    class MemcachedApi
            : public BaseQsciApi
    {
        Q_OBJECT
    public:
        explicit MemcachedApi(QsciLexer* lexer);

        virtual void updateAutoCompletionList(const QStringList& context, QStringList& list);
        virtual QStringList callTips(const QStringList& context, int commas, QsciScintilla::CallTipsStyle style, QList<int>& shifts);
    };

    class MemcachedLexer
            : public BaseQsciLexer
    {
        Q_OBJECT
    public:
        explicit MemcachedLexer(QObject* parent = 0);

        virtual const char* language() const;
        virtual const char* version() const;
        virtual const char* basedOn() const;

        virtual std::vector<uint32_t> supportedVersions() const;
        virtual uint32_t commandsCount() const;

        virtual void styleText(int start, int end);

    private:
        void paintCommands(const QString& source, int start);
    };
}

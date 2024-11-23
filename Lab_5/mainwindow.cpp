#include "mainwindow.h"
#include "ui_mainwindow.h"

QTemporaryFile MainWindow::tempFile;

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent),
                                          ui(new Ui::MainWindow),
                                          editor(new QTextEdit),
                                          tableWidget(new QTableWidget),
                                          tableModified(false)
{
    ui->setupUi(this);
    ui->tabWidget->setTabsClosable(true);
    connect(ui->tabWidget, &QTabWidget::tabCloseRequested, this, &MainWindow::closeTab);

    connect(tableWidget, &QTableWidget::cellChanged, this, &MainWindow::onTableCellChanged);

    QWidget *centralWidget = new QWidget(this);
    this->setCentralWidget(centralWidget);
    QVBoxLayout *layout = new QVBoxLayout();
    layout->addWidget(ui->tabWidget);

    centralWidget->setLayout(layout);

    setupShortcuts();

    QTextDocument *document = editor->document();
    QTextCharFormat format;

    if (document->isEmpty())
    {
        format.setBackground(Qt::white);
        editor->setCurrentCharFormat(format);
    }
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_CreateNewFile_triggered()
{
    QTextEdit *newEdit = new QTextEdit(this);
    pageIndex = ui->tabWidget->addTab(newEdit, "Новый файл");
    ui->tabWidget->setCurrentIndex(pageIndex);
    QTextCharFormat format;
    format.setBackground(Qt::white);
    newEdit->setCurrentCharFormat(format);
    newEdit->document()->setModified(false);
}

void MainWindow::on_OpenFile_triggered()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("Открыть файл"), "", tr("Text Files (*.txt);;Table Files(*.csv);;All Files (*)"));

    if (fileName.isEmpty())
    {
        return;
    }

    if (fileName.endsWith(".csv", Qt::CaseInsensitive))
    {
        QFile file(fileName);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        {
            QMessageBox::warning(nullptr, QObject::tr("Ошибка"), QObject::tr("Не удалось открыть CSV файл"));
            return;
        }

        QTextStream in(&file);
        QStringList lines;
        while (!in.atEnd())
        {
            lines.append(in.readLine());
        }
        file.close();

        int rows = lines.size();
        int columns = !lines.isEmpty() ? lines.first().split(",").size() : 0;

        if (rows == 0 || columns == 0)
        {
            QMessageBox::warning(nullptr, QObject::tr("Ошибка"), QObject::tr("Файл CSV пуст или имеет неправильный формат"));
            return;
        }

        if (tableWidget)
        {
            tableWidget->clear();
            tableWidget->setRowCount(0);
            tableWidget->setColumnCount(0);
        }

        QTableWidget *newTableWidget = new QTableWidget(rows, columns);
        newTableWidget->setWindowTitle(fileName);
        if (!tableWidget)
        {
            QMessageBox::warning(nullptr, QObject::tr("Ошибка"), QObject::tr("Не удалось создать таблицу"));
            return;
        }

        for (int i = 0; i < rows; ++i)
        {
            QStringList cells = lines.at(i).split(",");
            if (cells.size() != columns)
            {
                QMessageBox::warning(nullptr, QObject::tr("Ошибка"), QObject::tr("Некорректный CSV файл: строки содержат разное количество столбцов"));
                delete newTableWidget;
                return;
            }

            for (int j = 0; j < cells.size(); ++j)
            {
                newTableWidget->setItem(i, j, new QTableWidgetItem(cells.at(j)));
            }
        }

        pageIndex = ui->tabWidget->addTab(newTableWidget, QFileInfo(fileName).fileName());
        ui->tabWidget->setCurrentIndex(pageIndex);
        tableWidget = qobject_cast<QTableWidget *>(ui->tabWidget->currentWidget());
        connect(newTableWidget, &QTableWidget::cellChanged, this, &MainWindow::onTableCellChanged);
    }
    else
    {
        QFile file(fileName);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        {

            QMessageBox::warning(nullptr, QObject::tr("Ошибка"), QObject::tr("Не удалось открыть файл"));
            return;
        }

        QTextStream in(&file);
        QString fileContent = in.readAll();
        file.close();

        QTextEdit *newEdit = new QTextEdit();
        newEdit->setText(fileContent);

        int pageIndex = ui->tabWidget->addTab(newEdit, QFileInfo(fileName).fileName());
        ui->tabWidget->setCurrentIndex(pageIndex);
        pageIndex = ui->tabWidget->currentIndex();

        editor = qobject_cast<QTextEdit *>(ui->tabWidget->currentWidget());
        loadTextSettings(fileName);
        editor->document()->setModified(false);
    }
    pageIndex = ui->tabWidget->currentIndex();
    ui->tabWidget->setTabToolTip(pageIndex, fileName);
}

void MainWindow::on_SaveFile_triggered()
{
    QWidget *currentWidget = ui->tabWidget->currentWidget();
    if (!currentWidget)
    {
        QMessageBox::warning(this, tr("Ошибка"), tr("Нет открытой вкладки для сохранения"));
        return;
    }

    // Определяем тип виджета
    editor = qobject_cast<QTextEdit *>(currentWidget);
    QTableWidget *tableWidget = qobject_cast<QTableWidget *>(currentWidget);

    QString filePath = ui->tabWidget->tabToolTip(ui->tabWidget->currentIndex()); // Получаем путь к файлу из tabToolTip

    if (editor)
    {
        // Обработка для текстового редактора
        if (!filePath.isEmpty())
        {
            // Если файл существует, сохраняем изменения без диалога
            QFile file(filePath);
            if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
            {
                QMessageBox::warning(nullptr, "Ошибка", "Не удалось сохранить текстовый файл");
                return;
            }

            QTextStream out(&file);
            out << editor->toPlainText();
            file.close();
            saveTextSettings(filePath);
        }
        else
        {
            // Если файл новый, вызываем диалог сохранения
            filePath = QFileDialog::getSaveFileName(this, tr("Сохранить файл"), "", tr("Text Files (*.txt);;All Files (*)"));
            if (filePath.isEmpty())
            {
                return;
            }

            QFile file(filePath);
            if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
            {
                QMessageBox::warning(nullptr, "Ошибка", "Не удалось сохранить текстовый файл");
                return;
            }

            QTextStream out(&file);
            out << editor->toPlainText();
            file.close();
            saveTextSettings(filePath);
            // Устанавливаем путь в качестве подсказки на вкладке
            ui->tabWidget->setTabToolTip(ui->tabWidget->currentIndex(), filePath);
            ui->tabWidget->setTabText(ui->tabWidget->currentIndex(), QFileInfo(filePath).fileName());
        }
        editor->document()->setModified(false); // Снимаем флаг изменения документа
    }
    else if (tableWidget && tableModified)
    {
        // Обработка для таблицы
        if (!filePath.isEmpty())
        {
            // Если файл существует, сохраняем изменения без диалога
            QFile file(filePath);
            if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
            {
                QMessageBox::warning(nullptr, QObject::tr("Ошибка"), QObject::tr("Не удалось открыть файл для записи"));
                return;
            }

            QTextStream out(&file);
            int rows = tableWidget->rowCount();
            int columns = tableWidget->columnCount();

            // Записываем данные таблицы в файл
            for (int i = 0; i < rows; ++i)
            {
                QStringList rowContents;
                for (int j = 0; j < columns; ++j)
                {
                    QTableWidgetItem *item = tableWidget->item(i, j);
                    if (item)
                    {
                        rowContents << item->text();
                    }
                    else
                    {
                        rowContents << ""; // Пустая ячейка
                    }
                }
                out << rowContents.join(",") << "\n";
            }

            file.close();
        }
        else
        {
            // Если файл новый, вызываем диалог сохранения
            filePath = QFileDialog::getSaveFileName(this, tr("Сохранить файл таблицы"), "", tr("CSV Files (*.csv);;All Files (*)"));
            if (filePath.isEmpty())
            {
                return;
            }

            QFile file(filePath);
            if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
            {
                QMessageBox::warning(nullptr, QObject::tr("Ошибка"), QObject::tr("Не удалось открыть файл для записи"));
                return;
            }

            QTextStream out(&file);
            int rows = tableWidget->rowCount();
            int columns = tableWidget->columnCount();

            // Записываем данные таблицы в файл
            for (int i = 0; i < rows; ++i)
            {
                QStringList rowContents;
                for (int j = 0; j < columns; ++j)
                {
                    QTableWidgetItem *item = tableWidget->item(i, j);
                    if (item)
                    {
                        rowContents << item->text();
                    }
                    else
                    {
                        rowContents << ""; // Пустая ячейка
                    }
                }
                out << rowContents.join(",") << "\n";
            }

            file.close();
            // Устанавливаем путь в качестве подсказки на вкладке
            ui->tabWidget->setTabToolTip(ui->tabWidget->currentIndex(), filePath);
            ui->tabWidget->setTabText(ui->tabWidget->currentIndex(), QFileInfo(filePath).fileName());
            tableModified = false;
        }
    }
    else
    {
        QMessageBox::warning(this, tr("Ошибка"), tr("Текущая вкладка не поддерживает сохранение"));
    }
    tableModified = false;
}

void MainWindow::on_SaveFileAs_triggered()
{
    QWidget *currentWidget = ui->tabWidget->currentWidget();
    if (!currentWidget)
    {
        QMessageBox::warning(this, tr("Ошибка"), tr("Нет открытой вкладки для сохранения"));
        return;
    }

    editor = qobject_cast<QTextEdit *>(currentWidget);
    QTableWidget *tableWidget = qobject_cast<QTableWidget *>(currentWidget);

    QString filePath;
    if (tableWidget)
    {
        // Если активна таблица
        filePath = QFileDialog::getSaveFileName(this, tr("Сохранить файл таблицы как"), "", tr("CSV Files (*.csv);;All Files (*)"));
        if (filePath.isEmpty())
            return;

        QFile file(filePath);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        {
            QMessageBox::warning(this, tr("Ошибка"), tr("Не удалось открыть файл для записи"));
            return;
        }

        QTextStream out(&file);
        int rows = tableWidget->rowCount();
        int columns = tableWidget->columnCount();

        for (int i = 0; i < rows; ++i)
        {
            QStringList rowContents;
            for (int j = 0; j < columns; ++j)
            {
                QTableWidgetItem *item = tableWidget->item(i, j);
                rowContents << (item ? item->text() : "");
            }
            out << rowContents.join(",") << "\n";
        }

        file.close();
        ui->tabWidget->setTabToolTip(ui->tabWidget->currentIndex(), filePath);
        ui->tabWidget->setTabText(ui->tabWidget->currentIndex(), QFileInfo(filePath).fileName());
    }
    else if (editor)
    {
        // Если активен текстовый редактор
        filePath = QFileDialog::getSaveFileName(this, tr("Сохранить файл как"), "", tr("Text Files (*.txt);;All Files (*)"));
        if (filePath.isEmpty())
            return;

        QFile file(filePath);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        {
            QMessageBox::warning(this, tr("Ошибка"), tr("Не удалось сохранить файл"));
            return;
        }

        QTextStream out(&file);
        out << editor->toPlainText();
        file.close();

        ui->tabWidget->setTabToolTip(ui->tabWidget->currentIndex(), filePath);
        ui->tabWidget->setTabText(ui->tabWidget->currentIndex(), QFileInfo(filePath).fileName());
    }
}

void MainWindow::closeTab(int index)
{
    QWidget *widget = ui->tabWidget->widget(index);

    if (widget)
    {
        // Попытка преобразования в QTextEdit
        QTextEdit *editor = qobject_cast<QTextEdit *>(widget);
        QTableWidget *table = qobject_cast<QTableWidget *>(widget);
        QString filePath = ui->tabWidget->tabToolTip(index);

        // Проверка для QTextEdit
        if (editor && !editor->document()->isModified())
        {
            ui->tabWidget->removeTab(index);
            editor->deleteLater(); // Используем deleteLater() вместо delete
        }
        // Проверка для QTableWidget
        else if (table && !tableModified)
        {
            ui->tabWidget->removeTab(index);
            table->deleteLater(); // Используем deleteLater() вместо delete
        }
        else
        {
            // Диалоговое окно для подтверждения действий
            QMessageBox::StandardButton reply;
            reply = QMessageBox::question(this, "Закрыть вкладку",
                                          "Этот файл содержит несохранённые изменения. Сохранить перед закрытием?",
                                          QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);

            if (reply == QMessageBox::Yes)
            {
                // Реализуем сохранение файла перед закрытием
                on_SaveFile_triggered();
                ui->tabWidget->removeTab(index);
                widget->deleteLater(); // Используем deleteLater() вместо delete
            }
            else if (reply == QMessageBox::No)
            {
                ui->tabWidget->removeTab(index);
                widget->deleteLater(); // Используем deleteLater() вместо delete
            }
            // Если Cancel, ничего не делаем
        }
    }
}

void MainWindow::setupShortcuts()
{
    ui->actionCreateNewFile->setShortcut(QKeySequence::New);
    ui->actionOpenFile->setShortcut(QKeySequence::Open);
    ui->actionSaveFile->setShortcut(QKeySequence::Save);
    ui->actionSaveFileAs->setShortcut(QKeySequence::SaveAs);
    ui->actionCopy->setShortcut(QKeySequence::Copy);
    ui->actionClear->setShortcut(QKeySequence("Ctrl+Del"));
    ui->actionCut->setShortcut(QKeySequence::Cut);
    ui->actionSearch->setShortcut(QKeySequence::Find);
    ui->actionPaste->setShortcut(QKeySequence::Paste);
    ui->actionReplace->setShortcut(QKeySequence::Replace);
    ui->actionUndo->setShortcut(QKeySequence::Undo);
    ui->actionRedo->setShortcut(QKeySequence::Redo);
}

void MainWindow::on_Search_triggered()
{
    // Получаем текущий виджет
    QWidget *currentWidget = ui->tabWidget->currentWidget();
    editor = qobject_cast<QTextEdit *>(currentWidget);

    if (!editor)
    {
        QMessageBox::warning(this, tr("Ошибка"), tr("Текущая вкладка не поддерживает поиск. Создайте файл с текстом"));
        return;
    }
    editor->moveCursor(QTextCursor::Start);

    // Создаем диалоговое окно
    QDialog searchDialog(this);
    searchDialog.setWindowTitle("Поиск");

    // Элементы интерфейса
    QVBoxLayout *layout = new QVBoxLayout(&searchDialog);

    // Поле для текста поиска
    QLineEdit *searchLineEdit = new QLineEdit(&searchDialog);
    layout->addWidget(new QLabel("Введите текст для поиска:", &searchDialog));
    layout->addWidget(searchLineEdit);

    // Чекбоксы для учета регистра и полного слова
    QCheckBox *caseSensitiveCheckBox = new QCheckBox("Учитывать регистр", &searchDialog);
    layout->addWidget(caseSensitiveCheckBox);

    QCheckBox *wholeWordCheckBox = new QCheckBox("Искать только полные слова", &searchDialog);
    layout->addWidget(wholeWordCheckBox);

    // Добавляем кнопки "Следующее" и "Предыдущее"
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    QPushButton *nextButton = new QPushButton("Следующее", &searchDialog);
    QPushButton *prevButton = new QPushButton("Предыдущее", &searchDialog);
    buttonLayout->addWidget(prevButton);
    buttonLayout->addWidget(nextButton);
    layout->addLayout(buttonLayout);

    QPushButton *closeButton = new QPushButton("Закрыть", &searchDialog);
    layout->addWidget(closeButton);

    QRegularExpression cyrillicRegex("[\\p{Cyrillic}]");
    QRegularExpression latinRegex("[a-zA-Z]");

    //     Лямбда-функция для поиска текста
    auto search = [&](bool forward)
    {
        QString searchText = searchLineEdit->text();

        if (searchText.isEmpty())
        {
            QMessageBox::information(&searchDialog, "Поиск", "Введите текст для поиска.");
            return;
        }

        // Устанавливаем флаги поиска
        QTextDocument::FindFlags findFlags;
        if (caseSensitiveCheckBox->isChecked())
        {
            findFlags |= QTextDocument::FindCaseSensitively;
        }

        // Если ищем назад, добавляем флаг FindBackward
        QTextCursor cursor = editor->textCursor();
        QTextDocument *document = editor->document();

        // Если ищем назад
        if (!forward)
        {
            findFlags |= QTextDocument::FindBackward;

            // Проверяем позицию курсора и сдвигаем его на один символ назад, чтобы избежать повторного поиска
            int currentPosition = cursor.position();
            if (currentPosition > 0)
            {
                cursor.movePosition(QTextCursor::PreviousCharacter);
                editor->setTextCursor(cursor);
            }
        }

        // Проверяем, установлен ли флажок "Искать только полные слова"
        if (wholeWordCheckBox->isChecked())
        {
            // Используем регулярное выражение для поиска только целых слов
            //            QRegularExpression regex("\\b" + QRegularExpression::escape(searchText) + "\\b");
            QRegularExpression regex;
            if (cyrillicRegex.match(searchText).hasMatch())
            {
                regex.setPattern("((?<![\\p{L}\\d_])" + QRegularExpression::escape(searchText) + "(?![\\p{L}\\d_]))");

                //                regex.setPattern("(?<!\\w)" + QRegularExpression::escape(searchText) + "(?!\\w)");
            }
            else
            {
                regex.setPattern("\\b" + QRegularExpression::escape(searchText) + "\\b");
                //                Метасимвол \b (граница слова) может работать корректно с латинскими символами, но при поиске по кириллице он может некорректно обрабатывать границы слов.
                //                Это связано с тем, что \b ориентирован на латинские символы и цифры, и кириллические символы могут восприниматься неправиль
            }

            // Опции для учета регистра в регулярном выражении
            QRegularExpression::PatternOption caseOption = caseSensitiveCheckBox->isChecked()
                                                               ? QRegularExpression::NoPatternOption
                                                               : QRegularExpression::CaseInsensitiveOption;

            regex.setPatternOptions(caseOption);

            // Поиск с использованием регулярного выражения
            QTextCursor foundCursor = document->find(regex, cursor, findFlags);

            if (foundCursor.isNull())
            {
                QMessageBox::information(&searchDialog, "Поиск", "Текст не найден.");
            }
            else
            {
                editor->setStyleSheet("selection-background-color: blue; selection-color: white");
                editor->setTextCursor(foundCursor);
            }
        }
        else
        {
            // Обычный поиск без проверки на полные слова
            QTextCursor cursor = editor->textCursor();

            // Для обратного поиска нужно вручную сдвигать курсор назад перед поиском
            editor->setStyleSheet("selection-background-color: blue; selection-color: white");
            if (!forward && cursor.position() > 0)
            {
                cursor.movePosition(QTextCursor::PreviousCharacter);
                editor->setTextCursor(cursor);
            }

            if (!editor->find(searchText, findFlags))
            {
                QMessageBox::information(&searchDialog, "Поиск", "Текст не найден.");
            }
        }
    };

    // Соединяем кнопки "Следующее" и "Предыдущее" с действиями
    connect(nextButton, &QPushButton::clicked, [&]()
            { search(true); });
    connect(prevButton, &QPushButton::clicked, [&]()
            { search(false); });
    connect(closeButton, &QPushButton::clicked, &searchDialog, &QDialog::accept);

    // Показываем диалог
    searchDialog.exec();
}

void MainWindow::on_Replace_triggered()
{
    // Получаем текущий виджет
    QWidget *currentWidget = ui->tabWidget->currentWidget();
    editor = qobject_cast<QTextEdit *>(currentWidget);

    if (!editor)
    {
        QMessageBox::warning(this, tr("Ошибка"), tr("Текущая вкладка не поддерживает поиск"));
        return;
    }

    // Создаем диалоговое окно
    QDialog replaceDialog(this);
    replaceDialog.setWindowTitle("Поиск и замена");

    // Элементы интерфейса
    QVBoxLayout *layout = new QVBoxLayout(&replaceDialog);

    // Поле для текста поиска
    QLineEdit *searchLineEdit = new QLineEdit(&replaceDialog);
    layout->addWidget(new QLabel("Введите текст для поиска:", &replaceDialog));
    layout->addWidget(searchLineEdit);

    // Поле для текста замены
    QLineEdit *replaceLineEdit = new QLineEdit(&replaceDialog);
    layout->addWidget(new QLabel("Введите текст для замены:", &replaceDialog));
    layout->addWidget(replaceLineEdit);

    // Чекбоксы для учета регистра и полного слова
    QCheckBox *caseSensitiveCheckBox = new QCheckBox("Учитывать регистр", &replaceDialog);
    layout->addWidget(caseSensitiveCheckBox);

    QCheckBox *wholeWordCheckBox = new QCheckBox("Искать только полные слова", &replaceDialog);
    layout->addWidget(wholeWordCheckBox);

    // Кнопки замены и закрытия
    QPushButton *replaceButton = new QPushButton("Заменить все", &replaceDialog);
    layout->addWidget(replaceButton);

    QPushButton *closeButton = new QPushButton("Закрыть", &replaceDialog);
    layout->addWidget(closeButton);

    // Переменные для хранения флагов поиска
    QTextDocument::FindFlags findFlags;

    // Лямбда-функция для поиска и замены всех совпадений
    auto replaceAll = [&]()
    {
        QString searchText = searchLineEdit->text();
        QString replaceText = replaceLineEdit->text();

        if (searchText.isEmpty())
        {
            QMessageBox::information(&replaceDialog, "Поиск", "Введите текст для поиска.");
            return;
        }

        // Настройка флагов поиска
        findFlags = caseSensitiveCheckBox->isChecked() ? QTextDocument::FindCaseSensitively : QTextDocument::FindFlags();
        if (wholeWordCheckBox->isChecked())
        {
            findFlags |= QTextDocument::FindWholeWords;
        }

        // Перемещаем курсор в начало документа
        editor->moveCursor(QTextCursor::Start);

        bool isReplaced = false;

        // Поиск и замена всех совпадений
        while (editor->find(searchText, findFlags))
        {
            QTextCursor cursor = editor->textCursor();
            cursor.insertText(replaceText); // Замена текста
            isReplaced = true;
        }

        // Сообщение, если совпадений не найдено
        if (!isReplaced)
        {
            QMessageBox::information(&replaceDialog, "Заменить", "Текст для замены не найден.");
        }
    };

    // Соединение кнопок с действиями
    connect(replaceButton, &QPushButton::clicked, replaceAll);
    connect(closeButton, &QPushButton::clicked, &replaceDialog, &QDialog::accept);

    // Показываем диалог
    replaceDialog.exec();
}

void MainWindow::on_Clear_triggered()
{
    int pageIndex = ui->tabWidget->currentIndex();
    QWidget *currentWidget = ui->tabWidget->widget(pageIndex);
    QTextEdit *editor = qobject_cast<QTextEdit *>(currentWidget);

    if (!editor)
    {
        return;
    }

    if (!tempFile.isOpen() && !tempFile.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        return; // Cannot open temporary file
    }

    tempFile.write(editor->document()->toPlainText().toUtf8());
    tempFile.flush();
    tempFile.close();

    editor->document()->clear();
    editor->document()->setModified(true);
}

void MainWindow::on_Undo_triggered()
{
    if (editor)
    {
        if (this->tempFile.exists())
        {
            if (this->tempFile.open())
            {
                editor->undo();
                QString saved = QString::fromUtf8(this->tempFile.readAll());
                editor->document()->setPlainText(saved);
                this->tempFile.close();
            }
        }
        else
        {
            editor->document()->undo();
        }
    }
}

void MainWindow::onCopyTriggered()
{
    if (auto currentWidget = ui->tabWidget->currentWidget()) {
        if (auto textEdit = qobject_cast<QTextEdit*>(currentWidget)) {
            if (textEdit->textCursor().hasSelection()) {
                textEdit->copy();
            }
        }
    }
}

void MainWindow::onPasteTriggered()
{
    if (auto currentWidget = ui->tabWidget->currentWidget()) {
        if (auto textEdit = qobject_cast<QTextEdit*>(currentWidget)) {
            textEdit->paste();
        }
    }
}

void MainWindow::on_Cut_triggered()
{
    if (auto const currentWidget = ui->tabWidget->currentWidget()) {
        if (auto textEdit = qobject_cast<QTextEdit*>(currentWidget)) {
            if (textEdit->textCursor().hasSelection()) {
                textEdit->cut();
            }
        }
    }
}

void MainWindow::on_Redo_triggered()
{
    if (auto const currentWidget = ui->tabWidget->currentWidget()) {
        if (auto textEdit = qobject_cast<QTextEdit*>(currentWidget)) {
            textEdit->document()->redo();
        }
    }
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    // Флаг, который определяет, нужно ли продолжать закрытие
    bool shouldClose = true;

    // Начнём с конца списка вкладок, чтобы корректно закрывать их без сбоя счётчика
    for (int i = ui->tabWidget->count() - 1; i >= 0; --i)
    {
        QWidget *currentWidget = ui->tabWidget->widget(i);
        editor = qobject_cast<QTextEdit *>(currentWidget);

        if (editor && editor->document()->isModified())
        {
            // Если есть несохранённые изменения
            QString fileName = ui->tabWidget->tabToolTip(i);

            // Спрашиваем пользователя
            QMessageBox::StandardButton reply = QMessageBox::question(
                this,
                tr("Сохранить изменения"),
                tr("Файл \"%1\" содержит несохранённые изменения. Сохранить?").arg(fileName.isEmpty() ? "Новый файл" : QFileInfo(fileName).fileName()),
                QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);

            if (reply == QMessageBox::Yes)
            {
                // Если пользователь выбрал сохранить
                if (fileName.isEmpty())
                {
                    on_SaveFileAs_triggered();
                    shouldClose = true;
                }
                else
                {
                    on_SaveFile_triggered();
                    shouldClose = true;
                }

                // Закрываем вкладку после сохранения
                ui->tabWidget->removeTab(i);
                delete currentWidget;
            }
            else if (reply == QMessageBox::Cancel)
            {
                // Пользователь отменил закрытие
                shouldClose = false;
                break;
            }
            else if (reply == QMessageBox::No)
            {
                // Пользователь решил не сохранять изменения
                editor->document()->setModified(false); // Отменяем изменения
                ui->tabWidget->removeTab(i);            // Закрываем вкладку без сохранения
                delete currentWidget;
            }
        }
    }

    // Если разрешено закрытие, то вызываем стандартное закрытие
    if (shouldClose)
    {
        event->accept(); // Закрываем окно
    }
    else
    {
        event->ignore(); // Останавливаем закрытие
    }
}

void MainWindow::on_Palette_triggered()
{
    // Получаем текущий редактор
    editor = qobject_cast<QTextEdit *>(ui->tabWidget->currentWidget());
    if (!editor)
        return; // Если нет активного редактора, выходим

    QTextCursor cursor = editor->textCursor();

    // Получаем текущий цвет текста и фона
    auto currentForegroundColor = cursor.charFormat().foreground().color();
    auto currentBackgroundColor = cursor.charFormat().background().color();

    qDebug() << "currentBackgroundColor: " << currentBackgroundColor;

    // Открываем диалог выбора цвета текста
    QColor newTextColor = QColorDialog::getColor(currentForegroundColor, this, tr("Выберите цвет текста"));

    // Открываем диалог выбора цвета фона
    QColor newBackgroundColor = QColorDialog::getColor(currentBackgroundColor, this, tr("Выберите цвет фона"));

    // Если текстовый цвет не выбран, оставляем текущий или устанавливаем чёрный по умолчанию
    if (!newTextColor.isValid())
    {
        newTextColor = currentForegroundColor.isValid() ? currentForegroundColor : QColor(Qt::black);
    }

    // Если цвет фона не выбран или прозрачный, устанавливаем белый по умолчанию
    if (!newBackgroundColor.isValid() || newBackgroundColor.alpha() == 0)
    {
        newBackgroundColor = QColor(Qt::white); // Устанавливаем белый фон
    }

    // Проверка на совпадение цвета текста и фона
    if (newTextColor == newBackgroundColor)
    {
        // Если цвет фона светлый, текст должен быть чёрным, если тёмный — белым
        newTextColor = (newBackgroundColor.lightness() > 128) ? QColor(Qt::black) : QColor(Qt::white);
    }

    // Сохранение новых цветов
    textColor = newTextColor;
    backgroundColor = newBackgroundColor;

    // Устанавливаем цвета для выделенного текста или всего редактора
    QTextCharFormat format;
    format.setForeground(textColor);       // Устанавливаем цвет текста
    format.setBackground(backgroundColor); // Устанавливаем цвет фона

    // Если есть выделение текста, применяем формат только к выделенному тексту
    if (cursor.hasSelection())
    {
        cursor.mergeCharFormat(format);
    }
    else
    {
        // Если текста не выделено, применяем формат ко всей строке
        editor->setCurrentCharFormat(format);
        qDebug() << "editor->backgroundRole() " << editor->backgroundRole();
    }
}

void MainWindow::on_FontAndSize_triggered()
{
    // Проверяем текущий редактор
    editor = qobject_cast<QTextEdit *>(ui->tabWidget->currentWidget());
    if (!editor)
        return; // Если нет активного редактора, прерываем выполнение

    bool ok;
    // Открываем диалог выбора шрифта
    QFont font = QFontDialog::getFont(&ok, editor->currentFont(), this, tr("Выберите шрифт"));
    qDebug() << "Selected Font: " << font;

    if (ok)
    {
        // Сохраняем выбранный шрифт в переменной класса для дальнейшего использования
        currentFont = font;

        // Получаем текстовый курсор
        QTextCursor cursor = editor->textCursor();

        // Создаем формат для шрифта
        QTextCharFormat format;
        format.setFont(font);

        if (cursor.hasSelection())
        {
            // Если текст выделен, применяем шрифт к выделенному тексту
            qDebug() << "Applying Font to Selected Text: " << format.font();
            cursor.mergeCharFormat(format);
        }
        else
        {
            // Если текст не выделен, применяем шрифт для текущей строки
            qDebug() << "Applying Font to Future Text: " << format.font();
            editor->setCurrentCharFormat(format);
        }

        // Сохраняем изменения в документе редактора
        editor->document()->setModified(true);
    }
}

void MainWindow::saveTextSettings(const QString &filePath)
{
    editor = qobject_cast<QTextEdit *>(ui->tabWidget->currentWidget());
    if (!editor)
        return;

    QFileInfo fileInfo(filePath);
    QString relativePath = "../Laboratory_5/settings";

    QDir settingsDir(relativePath);
    if (!settingsDir.exists() && !settingsDir.mkpath("."))
    {
        qDebug() << "Unable to create directory: " << settingsDir.absolutePath();
        return;
    }

    QString jsonFilePath = settingsDir.absoluteFilePath(fileInfo.fileName() + ".html");

    // Извлекаем HTML код документа
    QString documentHtml = editor->toHtml();

    // Сохраняем HTML код в JSON объект
    QJsonObject settingsObject;
    settingsObject["html"] = documentHtml;

    QJsonDocument settingsDoc(settingsObject);
    QFile jsonFile(jsonFilePath);
    if (jsonFile.open(QIODevice::WriteOnly))
    {
        jsonFile.write(settingsDoc.toJson());
        jsonFile.close();
        qDebug() << "Settings saved to: " << jsonFilePath;
    }
    else
    {
        qDebug() << "Unable to open file for writing: " << jsonFilePath;
    }
}

void MainWindow::loadTextSettings(const QString &filePath)
{
    QTextEdit *editor = qobject_cast<QTextEdit *>(ui->tabWidget->currentWidget());
    if (!editor)
        return;

    QFileInfo fileInfo(filePath);
    QString relativePath = "../Laboratory_5/settings";
    QDir settingsDir(relativePath);
    QString jsonFilePath = settingsDir.absoluteFilePath(fileInfo.fileName() + ".html");

    QFile jsonFile(jsonFilePath);
    if (!jsonFile.open(QIODevice::ReadOnly))
    {
        qDebug() << "Unable to open file for reading: " << jsonFilePath;
        return;
    }

    QByteArray saveData = jsonFile.readAll();
    jsonFile.close();

    QJsonDocument loadDoc(QJsonDocument::fromJson(saveData));
    QJsonObject settingsObject = loadDoc.object();

    // Извлекаем HTML из JSON объекта
    QString documentHtml = settingsObject["html"].toString();

    // Очищаем текстовый редактор и вставляем HTML
    editor->clear();
    editor->setHtml(documentHtml);

    qDebug() << "Settings loaded from: " << jsonFilePath;
}

void MainWindow::on_Table_triggered()
{
    // Создаем диалог для выбора размера таблицы и способа вставки
    QDialog dialog(this);
    dialog.setWindowTitle(tr("Настройки таблицы"));

    // Надписи и элементы для выбора строк и столбцов
    QLabel *rowsLabel = new QLabel(tr("Строки:"), &dialog);
    QSpinBox *rowsSpinBox = new QSpinBox(&dialog);
    rowsSpinBox->setRange(1, 100);
    rowsSpinBox->setValue(3);

    QLabel *columnsLabel = new QLabel(tr("Столбцы:"), &dialog);
    QSpinBox *columnsSpinBox = new QSpinBox(&dialog);
    columnsSpinBox->setRange(1, 100);
    columnsSpinBox->setValue(3);

    // Радио-кнопки для выбора типа вставки
    QRadioButton *textEditorOption = new QRadioButton(tr("Вставить в текстовый редактор"), &dialog);
    QRadioButton *widgetOption = new QRadioButton(tr("Создать таблицу как виджет"), &dialog);
    textEditorOption->setChecked(true); // По умолчанию выбран текстовый редактор

    // Кнопки OK и Отмена
    QPushButton *okButton = new QPushButton(tr("OK"), &dialog);
    QPushButton *cancelButton = new QPushButton(tr("Отмена"), &dialog);

    // Сетка для расположения элементов диалога
    QGridLayout *layout = new QGridLayout;
    layout->addWidget(rowsLabel, 0, 0);
    layout->addWidget(rowsSpinBox, 0, 1);
    layout->addWidget(columnsLabel, 1, 0);
    layout->addWidget(columnsSpinBox, 1, 1);
    layout->addWidget(textEditorOption, 2, 0, 1, 2);
    layout->addWidget(widgetOption, 3, 0, 1, 2);
    layout->addWidget(okButton, 4, 0);
    layout->addWidget(cancelButton, 4, 1);
    dialog.setLayout(layout);

    // Связываем кнопки с завершением диалога
    connect(okButton, &QPushButton::clicked, &dialog, &QDialog::accept);
    connect(cancelButton, &QPushButton::clicked, &dialog, &QDialog::reject);

    // Если пользователь принял диалог
    if (dialog.exec() == QDialog::Accepted)
    {
        int rows = rowsSpinBox->value();
        int columns = columnsSpinBox->value();

        if (textEditorOption->isChecked())
        {
            // Вставляем таблицу как HTML в QTextEdit
            editor = qobject_cast<QTextEdit *>(ui->tabWidget->currentWidget());
            if (editor)
            {
                QString htmlTable = "<table border='1' cellspacing='2' cellpadding='4'>";
                for (int row = 0; row < rows; ++row)
                {
                    htmlTable += "<tr>";
                    for (int col = 0; col < columns; ++col)
                    {
                        htmlTable += "<td></td>";
                    }
                    htmlTable += "</tr>";
                }
                htmlTable += "</table>";

                // Вставляем таблицу в редактор
                editor->insertHtml(htmlTable);
            }
        }
        else if (widgetOption->isChecked())
        {
            // Создаем таблицу в виде QTableWidget
            tableWidget = new QTableWidget(rows, columns);
            tableWidget->setWindowTitle("Таблица");
            tableWidget->setEditTriggers(QAbstractItemView::DoubleClicked);
            tableModified = true;
            // Добавляем новую вкладку с таблицей в QTabWidget
            int index = ui->tabWidget->addTab(tableWidget, tr("Таблица %1").arg(ui->tabWidget->count() + 1));
            ui->tabWidget->setCurrentIndex(index);
        }
    }
}

void MainWindow::onTableCellChanged(int row, int column)
{
    Q_UNUSED(row);        // Если не используете эти параметры
    Q_UNUSED(column);     // Если не используете эти параметры
    tableModified = true; // Устанавливаем флаг изменений
}

void MainWindow::on_AddRow_triggered()
{
    QWidget *currentWidget = ui->tabWidget->currentWidget();
    if (QTableWidget *tableWidget = qobject_cast<QTableWidget *>(currentWidget))
    {
        // Добавляем строку в QTableWidget
        tableWidget->insertRow(tableWidget->rowCount());
        tableModified = true;
    }
    else if (QTextEdit *editor = qobject_cast<QTextEdit *>(currentWidget))
    {
        // Добавляем строку в таблицу в QTextEdit
        QTextCursor cursor = editor->textCursor();
        if (cursor.currentTable())
        {
            QTextTable *table = cursor.currentTable();
            table->appendRows(1);
        }
        else
        {
            QMessageBox::warning(this, "Ошибка", "Текущая ячейка не является таблицей.");
        }
    }
}

void MainWindow::on_AddColumn_triggered()
{
    QWidget *currentWidget = ui->tabWidget->currentWidget();
    if (QTableWidget *tableWidget = qobject_cast<QTableWidget *>(currentWidget))
    {
        // Добавляем столбец в QTableWidget
        tableWidget->insertColumn(tableWidget->columnCount());
        tableModified = true;
    }
    else if (QTextEdit *editor = qobject_cast<QTextEdit *>(currentWidget))
    {
        // Добавляем столбец в таблицу в QTextEdit
        QTextCursor cursor = editor->textCursor();
        if (cursor.currentTable())
        {
            QTextTable *table = cursor.currentTable();
            table->appendColumns(1);
        }
        else
        {
            QMessageBox::warning(this, "Ошибка", "Текущая ячейка не является таблицей.");
        }
    }
}

void MainWindow::on_DeleteRow_triggered()
{
    QWidget *currentWidget = ui->tabWidget->currentWidget();
    if (QTableWidget *tableWidget = qobject_cast<QTableWidget *>(currentWidget))
    {
        // Удаляем текущую строку в QTableWidget
        if (tableWidget->rowCount() > 1)
        {
            int currentRow = tableWidget->currentRow();
            if (currentRow != -1)
            {
                tableWidget->removeRow(currentRow);
            }
            else
            {
                QMessageBox::warning(this, "Ошибка", "Выберите строку для удаления.");
            }
        }
        tableModified = true;
    }
    else if (QTextEdit *editor = qobject_cast<QTextEdit *>(currentWidget))
    {
        // Удаляем строку в таблице в QTextEdit
        QTextCursor cursor = editor->textCursor();
        if (QTextTable *table = cursor.currentTable())
        {
            if (table->rows() > 1)
            {
                int currentRow = cursor.currentTable()->cellAt(cursor).row();
                table->removeRows(currentRow, 1);
            }
        }
        else
        {
            QMessageBox::warning(this, "Ошибка", "Текущая ячейка не является частью таблицы.");
        }
    }
}

void MainWindow::on_DeleteColumn_triggered()
{
    QWidget *currentWidget = ui->tabWidget->currentWidget();
    if (QTableWidget *tableWidget = qobject_cast<QTableWidget *>(currentWidget))
    {
        // Удаляем текущий столбец в QTableWidget
        if (tableWidget->columnCount() > 1)
        {
            int currentColumn = tableWidget->currentColumn();
            if (currentColumn != -1)
            {
                tableWidget->removeColumn(currentColumn);
            }
            else
            {
                QMessageBox::warning(this, "Ошибка", "Выберите столбец для удаления.");
            }
        }
        tableModified = true;
    }
    else if (QTextEdit *editor = qobject_cast<QTextEdit *>(currentWidget))
    {
        // Удаляем столбец в таблице в QTextEdit
        QTextCursor cursor = editor->textCursor();
        if (QTextTable *table = cursor.currentTable())
        {
            if (table->columns() > 1)
            {
                int currentColumn = cursor.currentTable()->cellAt(cursor).column();
                table->removeColumns(currentColumn, 1);
            }
        }
        else
        {
            QMessageBox::warning(this, "Ошибка", "Текущая ячейка не является частью таблицы.");
        }
    }
}
/**
* @file midicontrollerpresetfilehandler.cpp
* @author Sean Pappalardo spappalardo@mixxx.org
* @date Mon 9 Apr 2012
* @brief Handles loading and saving of MIDI controller presets.
*
*/

#include "midicontrollerpresetfilehandler.h"

#define DEFAULT_OUTPUT_MAX  1.0f
#define DEFAULT_OUTPUT_MIN  0.0f    // Anything above 0 is "on"
#define DEFAULT_OUTPUT_ON   0x7F
#define DEFAULT_OUTPUT_OFF  0x00

MidiControllerPresetFileHandler::MidiControllerPresetFileHandler()
        : ControllerPresetFileHandler() {
}

MidiControllerPresetFileHandler::~MidiControllerPresetFileHandler() {
}

ControllerPreset* MidiControllerPresetFileHandler::load(const QDomElement root,
                                                        const QString deviceName,
                                                        const bool forceLoad) {
    MidiControllerPreset* preset = new MidiControllerPreset();

    if (root.isNull())
        return preset;

    // Superclass handles script files
    addScriptFilesToMapping(root, deviceName, forceLoad, preset);

    /*
    // We actually need to load any script code now to verify function
    //  names against those in the XML
    qDebug() << "Supposed to load" << m_scriptFileNames.size() << "script files here";
    QStringList scriptFunctions;
    if (m_pEngine != NULL) {
        m_pEngine->loadScriptFiles(m_scriptFileNames);
        scriptFunctions = m_pEngine->getScriptFunctions();
    }
    else qWarning() << "No engine!";
    */

    QDomElement control = controller.firstChildElement("controls").firstChildElement("control");

    //Iterate through each <control> block in the XML
    while (!control.isNull()) {

        //Unserialize these objects from the XML
        QString strMidiStatus = control.firstChildElement("status").text();
        QString midiNo = control.firstChildElement("midino").text();

        bool ok = false;

        //Use QString with toInt base of 0 to auto convert hex values
        unsigned char midiStatusByte = strMidiStatus.toInt(&ok, 0);
        if (!ok) midiStatusByte = 0x00;

        unsigned char midiControl = midiNo.toInt(&ok, 0);
        if (!ok) midiControl = 0x00;

        QDomElement groupNode = control.firstChildElement("group");
        QDomElement keyNode = control.firstChildElement("key");
        QDomElement descriptionNode = control.firstChildElement("description");

        QString controlGroup = groupNode.text();
        QString controlKey = keyNode.text();
        QString controlDescription = descriptionNode.text();

        // Get options
        QDomElement optionsNode = control.firstChildElement("options").firstChildElement();

        MidiOptions options;
        options.all = 0;

        QString strMidiOption;
        while (!optionsNode.isNull()) {
            strMidiOption = optionsNode.nodeName().toLower();

            // "normal" is no options
            if (strMidiOption == "invert")   options.invert = true;
            if (strMidiOption == "rot64")    options.rot64 = true;
            if (strMidiOption == "rot64inv") options.rot64_inv = true;
            if (strMidiOption == "rot64fast")options.rot64_fast = true;
            if (strMidiOption == "diff")     options.diff = true;
            if (strMidiOption == "button")   options.button = true;
            if (strMidiOption == "switch")   options.sw = true;
            if (strMidiOption == "hercjog")  options.herc_jog = true;
            if (strMidiOption == "spread64") options.spread64 = true;
            if (strMidiOption == "selectknob")options.selectknob = true;
            if (strMidiOption == "soft-takeover") options.soft_takeover = true;
            if (strMidiOption == "script-binding") options.script = true;

            optionsNode = optionsNode.nextSiblingElement();
        }

        /*
        // Verify script functions are loaded
        if (options.script && !scriptFunctions.isEmpty() && scriptFunctions.indexOf(controlKey)==-1) {

            QString status = QString("0x%1").arg(midiStatusByte, 2, 16, QChar('0')).toUpper();
            QString byte2 = QString("0x%1").arg(midiControl, 2, 16, QChar('0')).toUpper();

            // If status is MIDI pitch, the 2nd byte is part of the payload so don't display it
            if ((midiStatusByte & 0xF0) == MIDI_PITCH_BEND) byte2 = "";

            QString errorLog = QString("MIDI script function \"%1\" not found. "
                                        "(Mapped to MIDI message %2 %3)")
                                        .arg(controlKey, status, byte2);

            if (m_bIsOutputDevice && debugging()) {
                    qCritical() << errorLog;
            }
            else {
                qWarning() << errorLog;
                ErrorDialogProperties* props = ErrorDialogHandler::instance()->newDialogProperties();
                props->setType(DLG_WARNING);
                props->setTitle(tr("MIDI script function not found"));
                props->setText(QString(tr("The MIDI script function '%1' was not "
                                "found in loaded scripts."))
                                .arg(controlKey));
                props->setInfoText(QString(tr("The MIDI message %1 %2 will not be bound."
                                "\n(Click Show Details for hints.)"))
                                    .arg(status, byte2));
                QString detailsText = QString(tr("* Check to see that the "
                    "function name is spelled correctly in the mapping "
                    "file (.xml) and script file (.js)\n"));
                detailsText += QString(tr("* Check to see that the script "
                    "file name (.js) is spelled correctly in the mapping "
                    "file (.xml)"));
                props->setDetails(detailsText);

                ErrorDialogHandler::instance()->requestErrorDialog(props);
            }
        } else
        */
        {
            // Add the static mapping
            QPair<ConfigKey, MidiOptions> target;
            target.first = ConfigKey(controlGroup, controlKey);
            target.second = options;

            unsigned char opCode = midiStatusByte & 0xF0;
            if (opCode >= 0xF0) opCode = midiStatusByte;

            bool twoBytes = true;
            switch (opCode) {
                case MIDI_SONG:
                case MIDI_NOTE_OFF:
                case MIDI_NOTE_ON:
                case MIDI_AFTERTOUCH:
                case MIDI_CC:
                    break;
                case MIDI_PITCH_BEND:
                case MIDI_SONG_POS:
                case MIDI_PROGRAM_CH:
                case MIDI_CH_AFTERTOUCH:
                default:
                    twoBytes = false;
                    break;
            }

            MidiKey key;
            key.status = midiStatusByte;

            if (twoBytes) key.control = midiControl;
            else {
                // Signifies that the second byte is part of the payload, default
                key.control = 0xFF;
            }
/*                qDebug() << "New mapping:" << QString::number(key.key, 16).toUpper()
                        << QString::number(key.status, 16).toUpper()
                        << QString::number(key.control, 16).toUpper()
                        << target.first.group << target.first.item;
*/
            preset.mappings.insert(key.key, target);
            // Notify the GUI and anyone else who cares
//                 emit(newMapping());  // TODO
        }
        control = control.nextSiblingElement("control");
    }

    qDebug() << "MidiControllerPresetFileHandler: Input mapping parsing complete.";

    // Parse static output mappings

    QDomElement output = controller.firstChildElement("outputs").firstChildElement("output");

    //Iterate through each <control> block in the XML
    while (!output.isNull()) {
        //Unserialize the ConfigKey components from the XML
        QDomElement groupNode = output.firstChildElement("group");
        QDomElement keyNode = output.firstChildElement("key");

        QString controlGroup = groupNode.text();
        QString controlKey = keyNode.text();

        // Unserialize output message from the XML
        MidiOutput outputMessage;

        QString midiStatus = output.firstChildElement("status").text();
        QString midiNo = output.firstChildElement("midino").text();
        QString midiOn = output.firstChildElement("on").text();
        QString midiOff = output.firstChildElement("off").text();

        bool ok = false;

        //Use QString with toInt base of 0 to auto convert hex values
        outputMessage.status = midiStatus.toInt(&ok, 0);
        if (!ok) outputMessage.status = 0x00;

        outputMessage.control = midiNo.toInt(&ok, 0);
        if (!ok) outputMessage.control = 0x00;

        outputMessage.on = midiOn.toInt(&ok, 0);
        if (!ok) outputMessage.on = DEFAULT_OUTPUT_ON;

        outputMessage.off = midiOff.toInt(&ok, 0);
        if (!ok) outputMessage.off = DEFAULT_OUTPUT_OFF;

        QDomElement minNode = output.firstChildElement("minimum");
        QDomElement maxNode = output.firstChildElement("maximum");

        ok = false;
        if (!minNode.isNull()) {
            outputMessage.min = minNode.text().toFloat(&ok);
        } else {
            ok = false;
        }

        if (!ok) //If not a float, or node wasn't defined
            outputMessage.min = DEFAULT_OUTPUT_MIN;

        if (!maxNode.isNull()) {
            outputMessage.max = maxNode.text().toFloat(&ok);
        } else {
            ok = false;
        }

        if (!ok) //If not a float, or node wasn't defined
            outputMessage.max = DEFAULT_OUTPUT_MAX;

        // END unserialize output

        // Add the static output mapping.
        /*
        qDebug() << "New output mapping:"
                        << QString::number(outputMessage.status, 16).toUpper()
                        << QString::number(outputMessage.control, 16).toUpper()
                        << QString::number(outputMessage.on, 16).toUpper()
                        << QString::number(outputMessage.off, 16).toUpper()
                        << controlGroup << controlKey;
        */
        // We use insertMulti because certain tricks are done with multiple
        //  entries for the same ConfigKey
        preset.outputMappings.insertMulti(ConfigKey(controlGroup, controlKey),outputMessage);

        output = output.nextSiblingElement("output");
    }

    qDebug() << "MidiPresetFileHandler: Output mapping parsing complete.";

    return preset;
}

QDomDocument MidiControllerPresetFileHandler::buildXML(const ControllerPreset& preset,
                                                       const QString deviceName)
{
    // The XML header and script stuff is handled by the superclass
    QDomDocument doc = ControllerPresetFileHandler::buildXML(preset, deviceName);

    QDomElement controller = doc.documentElement().firstChildElement("controller");
    QDomElement controls = doc.createElement("controls");
    controller.appendChild(controls);

    //Iterate over all of the command/control pairs in the input mapping
    QHashIterator<uint16_t, QPair<ConfigKey, MidiOptions> > it(preset.mappings);
    while (it.hasNext()) {
        it.next();

        QDomElement controlNode = doc.createElement("control");

        QDomText text;
        QDomDocument nodeMaker;
        QDomElement tagNode;

        QPair<ConfigKey, MidiOptions> target = it.value();
        MidiKey package;
        package.key = it.key();

//         qDebug() << QString::number(package.key, 16).toUpper()
//                  << QString::number(package.status, 16).toUpper()
//                  << QString::number(package.control, 16).toUpper()
//                  << target.first.group << target.first.item;
        mappingToXML(controlNode, target.first.group, target.first.item,
                     package.status, package.control);

        //Midi options
        QDomElement optionsNode = nodeMaker.createElement("options");
        MidiOptions options = it.value().second;

        // "normal" is no options
        if (options.all == 0) {
            QDomElement singleOption = nodeMaker.createElement("normal");
            optionsNode.appendChild(singleOption);
        }
        else {
            if (options.invert) {
                QDomElement singleOption = nodeMaker.createElement("invert");
                optionsNode.appendChild(singleOption);
            }
            if (options.rot64) {
                QDomElement singleOption = nodeMaker.createElement("rot64");
                optionsNode.appendChild(singleOption);
            }
            if (options.rot64_inv) {
                QDomElement singleOption = nodeMaker.createElement("rot64inv");
                optionsNode.appendChild(singleOption);
            }
            if (options.rot64_fast) {
                QDomElement singleOption = nodeMaker.createElement("rot64fast");
                optionsNode.appendChild(singleOption);
            }
            if (options.diff) {
                QDomElement singleOption = nodeMaker.createElement("diff");
                optionsNode.appendChild(singleOption);
            }
            if (options.button) {
                QDomElement singleOption = nodeMaker.createElement("button");
                optionsNode.appendChild(singleOption);
            }
            if (options.sw) {
                QDomElement singleOption = nodeMaker.createElement("switch");
                optionsNode.appendChild(singleOption);
            }
            if (options.herc_jog) {
                QDomElement singleOption = nodeMaker.createElement("hercjog");
                optionsNode.appendChild(singleOption);
            }
            if (options.spread64) {
                QDomElement singleOption = nodeMaker.createElement("spread64");
                optionsNode.appendChild(singleOption);
            }
            if (options.selectknob) {
                QDomElement singleOption = nodeMaker.createElement("selectknob");
                optionsNode.appendChild(singleOption);
            }
            if (options.soft_takeover) {
                QDomElement singleOption = nodeMaker.createElement("soft-takeover");
                optionsNode.appendChild(singleOption);
            }
            if (options.script) {
                QDomElement singleOption = nodeMaker.createElement("script-binding");
                optionsNode.appendChild(singleOption);
            }
        }

        controlNode.appendChild(optionsNode);

        //Add the control node we just created to the XML document in the proper spot
        controls.appendChild(controlNode);
    }

    QDomElement outputs = doc.createElement("outputs");
    controller.appendChild(outputs);

    //Iterate over all of the command/control pairs in the OUTPUT mapping
    QHashIterator<ConfigKey, MidiOutput> outIt(preset.outputMappings);
    while (outIt.hasNext()) {
        outIt.next();

        QDomElement outputNode = doc.createElement("output");
        MidiOutput outputPack = outIt.value();

        mappingToXML(outputNode, outIt.key().group, outIt.key().item, outputPack.status, outputPack.control);
        outputMappingToXML(outputNode, outputPack.on, outputPack.off, outputPack.max, outputPack.min);

        //Add the control node we just created to the XML document in the proper spot
        outputs.appendChild(outputNode);
    }

    return doc;
}

void MidiControllerPresetFileHandler::mappingToXML(QDomElement& parentNode, QString group,
                                  QString item, unsigned char status, unsigned char control) const
{
    QDomText text;
    QDomDocument nodeMaker;
    QDomElement tagNode;

    //Control object group
    tagNode = nodeMaker.createElement("group");
    text = nodeMaker.createTextNode(group);
    tagNode.appendChild(text);
    parentNode.appendChild(tagNode);

    //Control object name
    tagNode = nodeMaker.createElement("key"); //WTF worst name ever
    text = nodeMaker.createTextNode(item);
    tagNode.appendChild(text);
    parentNode.appendChild(tagNode);

    //MIDI status byte
    tagNode = nodeMaker.createElement("status");
    text = nodeMaker.createTextNode(
        QString("0x%1").arg(QString::number(status, 16).toUpper().rightJustified(2,'0')));
    tagNode.appendChild(text);
    parentNode.appendChild(tagNode);

    if (control != 0xFF) {
        //MIDI control number
        tagNode = nodeMaker.createElement("midino");
        text = nodeMaker.createTextNode(
            QString("0x%1").arg(QString::number(control, 16).toUpper().rightJustified(2,'0')));
        tagNode.appendChild(text);
        parentNode.appendChild(tagNode);
    }
}

void MidiControllerPresetFileHandler::outputMappingToXML(QDomElement& parentNode, unsigned char on,
                                        unsigned char off, float max, float min) const
{
    QDomText text;
    QDomDocument nodeMaker;
    QDomElement tagNode;

    // Third MIDI byte for turning on the LED
    if (on != DEFAULT_OUTPUT_ON) {
        tagNode = nodeMaker.createElement("on");
        text = nodeMaker.createTextNode(
            QString("0x%1").arg(QString::number(on, 16).toUpper().rightJustified(2,'0')));
        tagNode.appendChild(text);
        parentNode.appendChild(tagNode);
    }

    // Third MIDI byte for turning off the LED
    if (off != DEFAULT_OUTPUT_OFF) {
        tagNode = nodeMaker.createElement("off");
        text = nodeMaker.createTextNode(
            QString("0x%1").arg(QString::number(off, 16).toUpper().rightJustified(2,'0')));
        tagNode.appendChild(text);
        parentNode.appendChild(tagNode);
    }

    // Upper value, above which the 'off' value is sent
    //  (We don't bother writing it if it's the default value.)
    if (max != DEFAULT_OUTPUT_MAX) {
        tagNode = nodeMaker.createElement("maximum");
        QString value;
        value.setNum(max);
        text = nodeMaker.createTextNode(value);
        tagNode.appendChild(text);
        parentNode.appendChild(tagNode);
    }

    // Lower value, below which the 'off' value is sent
    //  (We don't bother writing it if it's the default value.)
    if (min != DEFAULT_OUTPUT_MIN) {
        tagNode = nodeMaker.createElement("minimum");
        QString value;
        value.setNum(min);
        text = nodeMaker.createTextNode(value);
        tagNode.appendChild(text);
        parentNode.appendChild(tagNode);
    }
}

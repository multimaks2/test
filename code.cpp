#pragma once
#include "SharedParamFMOD.hpp"
#include "CLuaFmod.h"

bool setDataEventInstanceForElement(SString elementName, SString eventName, FMOD::Studio::EventInstance* eventInstance)
{
    // Проверка наличия корневого объекта Massive
    if (fMassive)
    {
        // Поиск элемента с заданным именем
        auto elementIt = fMassive->mElementEvent.find(elementName);
        if (elementIt != fMassive->mElementEvent.end())
        {
            // Элемент найден. Получение ссылки на его содержимое.
            auto& element = elementIt->second;

            // Поиск значения для заданного имени события.
            auto eventIt = element.find(eventName);
            if (eventIt != element.end())
            {
                // Старое значение найдено. Удаление экземпляра звукового события.
                auto& oldEventInstance = eventIt->second;
                if (oldEventInstance && oldEventInstance != eventInstance)
                {
                    oldEventInstance->stop(FMOD_STUDIO_STOP_IMMEDIATE);
                    oldEventInstance->release();
                }
            }

            // Запись нового значения для заданного имени события.
            element[eventName] = eventInstance;
            return true;
        }
    }

    // Элемент не найден или корневой объект Massive не определен.
    return false;
}

FMOD::Studio::EventInstance* getDataEventInstanceForElement(SString elementName, SString eventName)
{
    if (fMassive)
    {
        auto it = fMassive->mElementEvent.find(elementName);
        if (it != fMassive->mElementEvent.end())
        {
            auto eventIt = it->second.find(eventName);
            if (eventIt != it->second.end())
            {
                // Элемент найден
                return eventIt->second;
            }
        }
    }
    // Элемент не найден
    return nullptr;
}

///////////////////////////////////////////////////////////////////////////
/////////////////////Эти функции пробросить в LUA//////////////////////////
///////////////////////////////////////////////////////////////////////////

bool loadFmodEventForElement(SString elementName, SString eventName)
{
    if (fMassive)
    {
        FMOD::Studio::EventDescription* pEventDescription = NULL;
        if (FMOD_OK == f_studio->getEvent(eventName, &pEventDescription))
        {
            if (pEventDescription)
            {
                FMOD::Studio::EventInstance* pEventInstance = NULL;
                pEventDescription->createInstance(&pEventInstance);
                if (pEventInstance)
                {
                    return setDataEventInstanceForElement(elementName, eventName, pEventInstance);
                }
            }
        }
    }
    return false;
}

bool releaseEventInstanceForElement(SString elementName, SString eventName)
{
    if (fMassive)
    {
        auto it = fMassive->mElementEvent.find(elementName);
        if (it != fMassive->mElementEvent.end())
        {
            auto eventIt = it->second.find(eventName);
            if (eventIt != it->second.end())
            {
                // Удаление экземпляра звукового события
                FMOD::Studio::EventInstance* eventInstance = eventIt->second;
                if (eventInstance)
                {
                    eventInstance->stop(FMOD_STUDIO_STOP_IMMEDIATE);            // резкая остановка
                    eventInstance->release();
                }
                // Удаление элемента из контейнера
                it->second.erase(eventIt);
                return true;
            }
        }
    }
    // Элемент не найден
    return false;
}

bool playFmodEventForElement(SString elementName, SString eventName)
{
    auto event = getDataEventInstanceForElement(elementName, eventName);
    if (nullptr != event)            // если не содержит ошибки
    {
        event->start();
        return true;
    }
    return false;
}

bool stopFmodEventForElement(SString elementName, SString eventName, bool bImmediate)
{
    FMOD_STUDIO_STOP_MODE eMode;
    eMode = bImmediate ? FMOD_STUDIO_STOP_IMMEDIATE : FMOD_STUDIO_STOP_ALLOWFADEOUT;
    auto event = getDataEventInstanceForElement(elementName, eventName);
    if (nullptr != event)            // если не содержит ошибки
    {
        event->stop(eMode);
        return true;
    }
    return false;
}

bool setPauseFmodEventForElement(SString elementName, SString eventName, bool pState)
{
    auto event = getDataEventInstanceForElement(elementName, eventName);
    if (nullptr != event)            // если не содержит ошибки
    {
        event->setPaused(pState);
        return true;
    }
    return false;
}

bool setFmodEventForElement3DPosition(SString elementName, SString eventName, CVector pos)
{
    if (isnan(pos.fX) || isnan(pos.fY) || isnan(pos.fZ))            // проверяем наличие NaN
    {
        // Если хотя бы одна координата содержит NaN, то сразу же возвращаем false.
        return false;
    }

    auto event = getDataEventInstanceForElement(elementName, eventName);
    if (nullptr != event)            // если не содержит ошибки
    {
        FMOD_3D_ATTRIBUTES attributes;
        memset(&attributes, 0, sizeof(attributes));
        attributes.position = ToFMOD_VECTOR(pos);
        event->set3DAttributes(&attributes);

        return true;
    }
    return false;
}

CVector getFmodEventForElement3DPosition(SString elementName, SString eventName)
{
    CVector pos;
    auto    event = getDataEventInstanceForElement(elementName, eventName);
    if (nullptr != event)            // если не содержит ошибки
    {
        FMOD_3D_ATTRIBUTES attributes;
        memset(&attributes, 0, sizeof(attributes));
        if (FMOD_OK == event->get3DAttributes(&attributes))
        {
            pos = ToCVector(attributes.position);            // преобразуем координаты из FMOD_VECTOR в CVector
        }
    }
    return pos;
}

// Установить громкость звука
bool setFmodEventVolume(SString elementName, SString eventName, float volume)
{
    auto event = getDataEventInstanceForElement(elementName, eventName);
    if (event != nullptr)                    // если экземпляр события создан успешно
    {
        event->setVolume(volume);            // установить громкость звука
        return true;
    }
    return false;
}

// Получить текущую громкость звука
float getFmodEventVolume(SString elementName, SString eventName)
{
    float volume = 0.0f;
    auto  event = getDataEventInstanceForElement(elementName, eventName);
    if (event != nullptr)                     // если экземпляр события создан успешно
    {
        event->getVolume(&volume);            // получить текущую громкость звука
    }
    return volume;
}
///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////
///////////Этот код выполняет обработку событий без владельца//////////////
///////////////////////////////////////////////////////////////////////////
bool setDataEventInstance(const SString& eventName, FMOD::Studio::EventInstance* pEventInstance)
{
    // Проверка переданного указателя на nullptr.
    if (pEventInstance == nullptr)
    {
        return false;
    }

    // Поиск значения для заданного имени события.
    auto it = fMassive->mEvents.find(eventName);
    if (it != fMassive->mEvents.end())
    {
        // Старое значение найдено. Удаление экземпляра звукового события.
        auto& oldEventInstance = it->second;
        if (oldEventInstance && oldEventInstance != pEventInstance)
        {
            oldEventInstance->stop(FMOD_STUDIO_STOP_IMMEDIATE);
            oldEventInstance->release();
        }
    }
    // Запись нового значения для заданного имени события.
    fMassive->mEvents[eventName] = pEventInstance;
    return true;
}

FMOD::Studio::EventInstance* getDataEventInstance(const SString& eventName)
{
    // Поиск значения для заданного имени события.
    auto it = fMassive->mEvents.find(eventName);
    if (it != fMassive->mEvents.end())
    {
        // Значение найдено. Возвращаем экземпляр звукового события.
        return it->second;
    }

    // Значение не найдено. Возвращаем nullptr.
    return nullptr;
}

bool loadFmodEvent(const SString& eventName)
{
    FMOD::Studio::EventDescription* eventDescription = nullptr;
    result = f_studio->getEvent(eventName, &eventDescription);
    if (result != FMOD_OK || eventDescription == nullptr)
    {
        // Обработка ошибки.
        return false;
    }

    FMOD::Studio::EventInstance* eventInstance = nullptr;
    eventDescription->createInstance(&eventInstance);
    if (eventInstance == nullptr)
    {
        // Обработка ошибки.
        return false;
    }

    return setDataEventInstance(eventName, eventInstance);
}

bool releaseFmodEvent(const SString& eventName)
{
    FMOD::Studio::EventInstance* eventInstance = getDataEventInstance(eventName);
    if (eventInstance != nullptr)
    {
        eventInstance->stop(FMOD_STUDIO_STOP_IMMEDIATE);
        eventInstance->release();
        return true;
    }
    // Возвращаем false, если указанное звуковое событие не найдено.
    return false;
}

bool playFmodEvent(const SString& eventName)
{
    FMOD::Studio::EventInstance* eventInstance = getDataEventInstance(eventName);
    if (eventInstance != nullptr)
    {
        eventInstance->start();
        return true;
    }

    // Возвращаем false, если указанное звуковое событие не найдено.
    return false;
}

bool stopFmodEvent(const SString& eventName)
{
    FMOD::Studio::EventInstance* eventInstance = getDataEventInstance(eventName);
    if (eventInstance != nullptr)
    {
        eventInstance->stop(FMOD_STUDIO_STOP_IMMEDIATE);
        return true;
    }

    // Возвращаем false, если указанное звуковое событие не найдено.
    return false;
}

bool setPauseFmodEvent(const SString& eventName, bool paused)
{
    FMOD::Studio::EventInstance* event = getDataEventInstance(eventName);
    if (event != nullptr)
    {
        event->setPaused(paused);
        return true;
    }
    // Возвращаем false, если указанное звуковое событие не найдено.
    return false;
}

bool getPauseFmodEvent(const SString& eventName)
{
    FMOD::Studio::EventInstance* eventInstance = getDataEventInstance(eventName);
    if (eventInstance != nullptr)
    {
        bool paused;
        eventInstance->getPaused(&paused);
        return paused;
    }

    // Возвращаем false, если указанное звуковое событие не найдено.
    return false;
}



bool setPosFmodEvent(SString eventName, CVector pos)
{
    if (isnan(pos.fX) || isnan(pos.fY) || isnan(pos.fZ))            // проверяем наличие NaN
    {
        // Если хотя бы одна координата содержит NaN, то сразу же возвращаем false.
        return false;
    }

    auto event = getDataEventInstance(eventName);
    if (nullptr != event)            // если не содержит ошибки
    {
        FMOD_3D_ATTRIBUTES attributes;
        memset(&attributes, 0, sizeof(attributes));
        attributes.position = ToFMOD_VECTOR(pos);
        event->set3DAttributes(&attributes);

        return true;
    }
    return false;
}
///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////

/*
Copyright (c) 2013-2016 Chukong Technologies Inc.
Copyright (c) 2017-2022 Xiamen Yaji Software Co., Ltd.

http://www.cocos.com

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated engine source code (the "Software"), a limited,
worldwide, royalty-free, non-assignable, revocable and non-exclusive license
to use Cocos Creator solely to develop games on your target platforms. You shall
not use Cocos Creator software for developing other software or tools that's
used for developing games. You are not granted to publish, distribute,
sublicense, and/or sell copies of Cocos Creator.

The software or tools in this License Agreement are licensed, not sold.
Xiamen Yaji Software Co., Ltd. reserves all rights not expressly granted to you.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

#pragma once

namespace Cocos::Meta {

// Typename
void removeParenthesis(std::pmr::string& name);
void convertTypename(std::pmr::string& name);
std::pmr::string convertTypename(std::string_view name,
    std::pmr::memory_resource* scratch);
bool validateTypename(std::string_view name);

// General Path
std::string_view parentPath(std::string_view path);

// TypePath
bool isTypePath(std::string_view typePath);
std::string_view extractScope(std::string_view name);
std::string_view getDependentPath(std::string_view scope, std::string_view typePath);

std::string_view getTemplateName(std::string_view instanceName);

void extractTemplate(std::string_view instanceName,
    std::pmr::string& name,
    std::pmr::vector<std::pmr::string>& parameters);

std::string_view extractName(std::string_view typePath);

// Struct
std::pmr::string camelToVariable(std::string_view name,
    std::pmr::memory_resource* scratch);

std::pmr::string getMemberName(std::string_view member,
    std::pmr::memory_resource* scratch);

// Syntax
bool isInstance(std::string_view name);

// Generation
std::pmr::string getCppPath(std::string_view typePath,
    std::pmr::memory_resource* mr);

std::pmr::string getVariableName(std::string_view typeName,
    std::pmr::memory_resource* mr);

std::string_view convertTag(std::string_view tagName);

std::pmr::string getTagName(std::string_view typeName,
    std::pmr::memory_resource* mr);

std::pmr::string getRelativePath(std::string_view current, std::string_view target,
    std::pmr::memory_resource* mr);

}

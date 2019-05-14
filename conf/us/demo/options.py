# -*- coding: utf-8 -*-
# Copyright (c) 2018 Baidu, Inc. All Rights Reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

"""
Options for generating conf of USKit
"""
options = {
    # server address
    # 服务器地址
    'redis_server' : '127.0.0.1:6379',
    'dmkit_server' : '127.0.0.1:8010',
    'unit_server' : 'https://aip.baidubce.com',

    # skill ids
    # 填写每个技能对应的技能id
    # 以下技能id仅作示例，请填写你自己的技能id
    # 查找你自己的技能id：http://ai.baidu.com/unit/v2#/sceneliblist
    'unit_skill_id' : {
        'weather' : '11111',
        'flight' : '22222',
        'talk' : '33333'
    },

    'dmkit_skill_id' : {
        'hotel' : '44444'
    },


    # rank order
    # 按照数组从左到右的优先级进行排序
    'rank' : ['hotel', 'flight', 'weather', 'talk'],

    # access token
    # 查看UNIT文档了解如何获取API Key和Secret Key: http://ai.baidu.com/docs#/Auth/top
    'api_key' : 'your_api_key',
    'secret_key' : 'your_secret_key'
}

options['skill_id'] = dict(options['unit_skill_id'].items() + options['dmkit_skill_id'].items())
options['unit_host'] = options['unit_server'].replace('https://', '').replace('http://', '')
options['skill_rank'] = ['{}_{}'.format(x, options['skill_id'][x]) for x in options['rank']]

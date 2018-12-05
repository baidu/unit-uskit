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
US Kit Conf generator
"""
from __future__ import print_function
from string import Template
from options import options

class ConfTemplate(Template):
    """
    String template with @ delimiter
    """
    delimiter = '@'


def generate_token_backend_conf(fout):
    """
    Generate token backend
    """
    with open('./conf_templates/token_backend.template') as fin:
        template = ConfTemplate(fin.read())
        print(template.substitute(options), file=fout)


def generate_redis_backend_conf(fout):
    """
    Generate redis backend
    """
    with open('./conf_templates/redis_backend.template') as fin:
        template = ConfTemplate(fin.read())
        print(template.substitute(options), file=fout)


def generate_dmkit_backend(fout):
    """
    Generate DM Kit backend
    """
    with open('./conf_templates/unit_backend.template') as backend_fin, \
         open('./conf_templates/unit_service.template') as service_fin:
        backend_template = ConfTemplate(backend_fin.read())
        service_template = ConfTemplate(service_fin.read())
        service_conf = []
        for skill, sid in options['dmkit_skill_id'].items():
            service_conf.append(service_template.substitute({
                'backend' : 'dmkit',
                'skill' : skill,
                'sid' : sid}))
        service_conf = '\n'.join(service_conf)
        print(backend_template.substitute({
            'backend' : 'dmkit',
            'server' : options['dmkit_server'],
            'url' : '/search',
            'host' : options['unit_host'],
            'service_conf' : service_conf}), file=fout)


def generate_unit_backend(fout):
    """
    Generate UNIT backend
    """
    with open('./conf_templates/unit_backend.template') as backend_fin, \
         open('./conf_templates/unit_service.template') as service_fin:
        backend_template = ConfTemplate(backend_fin.read())
        service_template = ConfTemplate(service_fin.read())
        service_conf = []
        for skill, sid in options['unit_skill_id'].items():
            service_conf.append(service_template.substitute({
                'backend' : 'unit',
                'skill' : skill,
                'sid' : sid}))
        service_conf = '\n'.join(service_conf)
        print(backend_template.substitute({
            'backend' : 'unit',
            'server' : options['unit_server'],
            'url' : '/rpc/2.0/unit/bot/chat',
            'host' : options['unit_host'],
            'service_conf' : service_conf}), file=fout)


def generate_backend_conf():
    """
    Generate backend.conf
    """
    print('generating backend conf...')
    with open('backend.conf', 'w') as fout:
        generate_token_backend_conf(fout)
        generate_redis_backend_conf(fout)
        generate_dmkit_backend(fout)
        generate_unit_backend(fout)


def generate_rank_conf():
    """
    Generate rank.conf
    """
    print('generating rank conf...')
    with open('./conf_templates/rank.conf.template') as fin, open('rank.conf', 'w') as fout:
        skill_rank = '\n'.join(['    order: "{}"'.format(x) for x in options['skill_rank']])
        template = ConfTemplate(fin.read())
        print(template.substitute({'skill_rank' : skill_rank}), file=fout)


def generate_flow_conf():
    """
    Generate flow.conf
    """
    print('generating flow conf...')
    with open('./conf_templates/flow.conf.template') as fin, open('flow.conf', 'w') as fout:
        recall_skill = '\n'.join(['    recall: "{}"'.format(x) for x in options['skill_rank']])
        template = ConfTemplate(fin.read())
        print(template.substitute({'recall_skill' : recall_skill}), file=fout)

if __name__ == "__main__":
    generate_backend_conf()
    generate_rank_conf()
    generate_flow_conf()
    print('done')

#!groovy

BuildPackage(
    sources: [
        original: 'all'
    ],
    dists: [
        'centos6',
        'centos7',
        'centos8',
        'bionic',
        'stretch',
        'buster',
        'bullseye',
        'focal'
    ],
    target_repos: [
        'wallarm-node': [
            release: '3.6',
            dev: true
        ]
    ],
    notify: [
        email: 'node-dev@wallarm.com'
    ]
)

#!groovy

BuildPackage(
    sources: [
        original: 'all'
    ],
    dists: [
        'centos6',
        'centos7',
        'trusty',
        'xenial',
        'bionic',
        'jessie',
        'stretch',
        'buster'
    ],
    repos: [
        'wallarm': [
		dev: true
	],
        'wallarm-node': [
	]
    ],
    notify: [
        email: 'node-dev@wallarm.com'
    ]
)

#!groovy

BuildPackage(
    sources: [
        original: 'all'
    ],
    dists: [
        'centos6',
        'centos7',
        'centos8',
        'trusty',
        'xenial',
        'bionic',
        'jessie',
        'stretch',
        'buster'
    ],
    target_repos: [
        'wallarm-node': [
		release: '2.15',
		dev: true
	]
    ],
    notify: [
        email: 'node-dev@wallarm.com'
    ]
)

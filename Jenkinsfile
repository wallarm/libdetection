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
    target_repos: [
        'wallarm-node': [
		release: '2.13',
		dev: true
	]
    ],
    notify: [
        email: 'node-dev@wallarm.com'
    ]
)
